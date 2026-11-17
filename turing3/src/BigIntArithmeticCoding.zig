//gemini might have written this cus idk about arithmetic coding yet

const std = @import("std");
const BigInt = std.math.big.int.Managed;
const LambdaExpression = @import("LambdaParser.zig").LambdaExpression;

// pub const LambdaExpression = union(enum) {
//     lambda: struct { expr: *LambdaExpression },
//     apply: struct { lexpr: *LambdaExpression, rexpr: *LambdaExpression },
//     variable: struct { value: u32 }, // 1-based De Bruijn index (1 .. depth)
// };

const EncodeError = error{ UnboundVar, OutOfMemory };

pub const BigIntRangeCoder = struct {
    fn walkEncode(
        expr: *const LambdaExpression,
        depth: u32,
        low: *BigInt,
        den: *BigInt,
        tmp: *BigInt,
    ) EncodeError!void {
        const menu: u32 = depth + 2;
        const choice: u32 = switch (expr.*) {
            .variable => |v| blk: {
                // Reject free/unbound variables (0 or > depth)
                if (v.value == 0 or v.value > depth) {
                    return EncodeError.UnboundVar;
                }
                break :blk v.value - 1;
            },
            .lambda => depth,
            .apply => depth + 1,
        };

        // low = low * menu + choice
        try tmp.set(menu);
        try low.mul(low, tmp);
        try tmp.set(choice);
        try low.add(low, tmp);

        // den = den * menu
        try tmp.set(menu);
        try den.mul(den, tmp);

        switch (expr.*) {
            .variable => {},
            .lambda => |l| try walkEncode(l.expr, depth + 1, low, den, tmp),
            .apply => |a| {
                try walkEncode(a.lexpr, depth, low, den, tmp);
                try walkEncode(a.rexpr, depth, low, den, tmp);
            },
        }
    }

    /// Encodes `expr` into a unique binary string (`'0'` / `'1'` slice owned by caller).
    /// Supports arbitrarily large ASTs and arbitrary output lengths.
    pub fn encode(expr: *const LambdaExpression, allocator: std.mem.Allocator) ![]u8 {
        var low = try BigInt.initSet(allocator, 0);
        defer low.deinit();

        var den = try BigInt.initSet(allocator, 1);
        defer den.deinit();

        var tmp = try BigInt.init(allocator);
        defer tmp.deinit();

        try walkEncode(expr, 0, &low, &den, &tmp);

        // Special case: `/(1)` has low == 0 -> shortest binary string in [0, 1/6) is ""
        if (low.eqlZero()) {
            return try allocator.dupe(u8, "");
        }

        // Find smallest bit length `L` such that a dyadic fraction k / 2^L lies strictly in
        // [low / den, (low + 1) / den), i.e., ceil(low * 2^L / den) < ceil((low + 1) * 2^L / den)
        var high_num = try low.clone();
        defer high_num.deinit();
        try tmp.set(1);
        try high_num.add(&high_num, &tmp); // high_num = low + 1

        var k_low = try BigInt.init(allocator);
        defer k_low.deinit();

        var k_high = try BigInt.init(allocator);
        defer k_high.deinit();

        var rem = try BigInt.init(allocator);
        defer rem.deinit();

        var len: usize = 0;
        while (true) {
            len += 1;
            // low_shifted = low * 2^len, high_shifted = (low + 1) * 2^len
            try low.shiftLeft(&low, 1);
            try high_num.shiftLeft(&high_num, 1);

            // k_low = ceil(low_shifted / den) = (low_shifted + den - 1) / den
            try k_low.add(&low, &den);
            try tmp.set(1);
            try k_low.sub(&k_low, &tmp);
            try k_low.divFloor(&rem, &k_low, &den);

            // k_high = ceil(high_shifted / den)
            try k_high.add(&high_num, &den);
            try k_high.sub(&k_high, &tmp);
            try k_high.divFloor(&rem, &k_high, &den);

            // If k_low < k_high, then k_low / 2^len is strictly inside [low/den, (low+1)/den)
            if (k_low.order(k_high) == .lt) {
                const raw_bits = try k_low.toString(allocator, 2, .lower);
                defer allocator.free(raw_bits);

                // Left-pad with '0' to exact length `len`
                const out = try allocator.alloc(u8, len);
                const pad = len - raw_bits.len;
                @memset(out[0..pad], '0');
                @memcpy(out[pad..], raw_bits);
                return out;
            }
        }
    }

    fn walkDecode(
        depth: u32,
        num: *BigInt,
        den: *BigInt,
        menu_big: *BigInt,
        choice_big: *BigInt,
        rem: *BigInt,
        allocator: std.mem.Allocator,
    ) !*LambdaExpression {
        const menu: u32 = depth + 2;
        try menu_big.set(menu);

        // choice = floor((num * menu) / den)
        try num.mul(num, menu_big);
        try num.divFloor(rem, num, den); // num becomes quotient (`choice`), rem becomes remainder
        const choice: u32 = try num.to(u32);

        // Update num = remainder so fraction becomes rem / den
        try num.copy(rem.toConst());
        // _ = choice_big;

        const node = try allocator.create(LambdaExpression);
        if (choice < depth) {
            node.* = .{ .variable = .{ .value = choice + 1 } };
        } else if (choice == depth) {
            node.* = .{ .lambda = .{
                .expr = try walkDecode(depth + 1, num, den, menu_big, choice_big, rem, allocator),
            } };
        } else {
            const lexpr = try walkDecode(depth, num, den, menu_big, choice_big, rem, allocator);
            const rexpr = try walkDecode(depth, num, den, menu_big, choice_big, rem, allocator);
            node.* = .{ .apply = .{ .lexpr = lexpr, .rexpr = rexpr } };
        }
        return node;
    }

    /// Decodes ANY binary string (of arbitrary length) into a valid closed `LambdaExpression`.
    pub fn decode(bits: []const u8, allocator: std.mem.Allocator) !*LambdaExpression {
        var num = try BigInt.initSet(allocator, 0);
        defer num.deinit();

        var den = try BigInt.initSet(allocator, 1);
        defer den.deinit();

        if (bits.len > 0) {
            try num.setString(2, bits);
            try den.shiftLeft(&den, bits.len);
        }

        var menu_big = try BigInt.init(allocator);
        defer menu_big.deinit();

        var choice_big = try BigInt.init(allocator);
        defer choice_big.deinit();

        var rem = try BigInt.init(allocator);
        defer rem.deinit();

        return try walkDecode(0, &num, &den, &menu_big, &choice_big, &rem, allocator);
    }
};
