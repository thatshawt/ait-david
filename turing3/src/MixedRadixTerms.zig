const std = @import("std");

const LambdaExpression = @import("LambdaParser.zig").LambdaExpression;

const ArrayList = std.ArrayList;

const BigIntManaged = std.math.big.int.Managed;

//TODO, this is a bijection from lambda term to binary strings...
// i want to add this because BLC does not cover all the binary strings when outputting...
// AI chat below of how to do it

// 1. The Menu of Choices at Depth d

// Walk through your lambda term from top to bottom. Keep a counter d for how many λs you have entered so far.
// At every node, you have d + 2 possible choices:
// • 0: A λ abstraction (increases d by 1)
// • 1: An application (M N) (keeps d the same, branches twice)
// • 2, 3, ..., d+1: A variable 1, 2, ..., d (stops this branch)
// (Note: When d = 0, there are no variables in scope yet, so the menu only has 2 choices: 0 or 1.)
// Example (λf. λx. x):
// 1. Outer λ at d = 0 → picked 0 out of 2 choices
// 2. Inner λ at d = 1 → picked 0 out of 3 choices
// 3. Variable 1 at d = 2 → picked 2 out of 4 choices
// Your term is now just a list of (choice, menu_size) pairs:
// [(0, 2), (0, 3), (2, 4)]

// 2. Pack the Choices into One Number (N)

// Think of this like converting hours, minutes, and seconds into total seconds—except each position has its own base (menu_size).
// Start with N = 0 and work backwards through your list:
// N = N * menu_size + (choice + 1)
// Using our example [(0, 2), (0, 3), (2, 4)] in reverse:
// 1. Start: N = 0
// 2. (2, 4): N = 0 * 4 + (2 + 1) = 3
// 3. (0, 3): N = 3 * 3 + (0 + 1) = 10
// 4. (0, 2): N = 10 * 2 + (0 + 1) = 21
// Subtract 1 at the very end so counting starts at 0: N = 20.
// (Why + 1? Because in normal base-10, 05 and 5 are the same number. Adding 1 acts like a "leading digit" so leading 0 choices are never lost!)

// 3. Turn N into Any Binary String

// To map N (0, 1, 2, 3, ...) to every binary string ("", "0", "1", "00", ...):
// 1. Add 1 to N (20 + 1 = 21)
// 2. Write it in binary (21 in binary is 10101)
// 3. Chop off the first 1 → "0101"

// Minimal Code (Encoder & Decoder)

// python
// def encode(term):
//     # 1. Collect (choice, menu_size) pairs
//     pairs = []
//     def walk(t, d=0):
//         menu = d + 2
//         if t == 'abs':
//             pairs.append((0, menu))
//             walk(t, d + 1)
//         elif t == 'app':
//             pairs.append((1, menu))
//             walk(t, d)
//             walk(t, d)
//         elif t == 'var':
//             pairs.append((t + 1, menu))
//     walk(term)

//     # 2. Pack into a single integer N
//     n = 0
//     for choice, menu in reversed(pairs):
//         n = n * menu + choice + 1

//     # 3. Convert to bijective binary string (N already has +1 built in!)
//     return bin(n)[3:]

const MixedRadixArgs = struct {
    const Self = @This();
    result: ?BigIntManaged = null, //
    temp: ?BigIntManaged = null,
    // temp2: ?BigIntManaged = null,
    allocator: std.mem.Allocator,

    fn init(self: *Self) void {
        self.result = BigIntManaged.initSet(self.allocator, 0) catch unreachable;
        self.temp = BigIntManaged.initSet(self.allocator, 0) catch unreachable;
    }

    fn deinit(self: *Self) void {
        self.result.?.deinit();
        self.temp.?.deinit();
    }
};
fn mixedRadixTraverseFn(expr: *LambdaExpression, argz: anytype, depth: u32) error{Yield}!void {
    var args: *MixedRadixArgs = argz;

    const menu: u32 = depth + 2;
    //AI CODEs
    switch (expr.*) {
        .variable => |varExpr| {
            const choice: u32 = varExpr.value - 1;
            // Leaf: result = result * menu + choice
            args.temp.?.set(menu) catch unreachable;
            args.result.?.mul(&args.result.?, &args.temp.?) catch unreachable;
            args.temp.?.set(choice) catch unreachable;
            args.result.?.add(&args.result.?, &args.temp.?) catch unreachable;
        },
        .lambda => {
            // Branch: result = (result + 1) * menu + depth
            args.temp.?.set(1) catch unreachable;
            args.result.?.add(&args.result.?, &args.temp.?) catch unreachable;
            args.temp.?.set(menu) catch unreachable;
            args.result.?.mul(&args.result.?, &args.temp.?) catch unreachable;
            args.temp.?.set(depth) catch unreachable;
            args.result.?.add(&args.result.?, &args.temp.?) catch unreachable;
        },
        .apply => {
            // Branch: result = (result + 1) * menu + (depth + 1)
            args.temp.?.set(1) catch unreachable;
            args.result.?.add(&args.result.?, &args.temp.?) catch unreachable;
            args.temp.?.set(menu) catch unreachable;
            args.result.?.mul(&args.result.?, &args.temp.?) catch unreachable;
            args.temp.?.set(depth + 1) catch unreachable;
            args.result.?.add(&args.result.?, &args.temp.?) catch unreachable;
        },
    }

    // const choice: u32 =
    //     switch (expr.*) {
    //         .lambda => depth,
    //         .apply => depth + 1,
    //         .variable => |varExpr| varExpr.value - 1,
    //     };

    // args.temp.?.set(menu) catch unreachable;
    // args.result.?.mul(&args.result.?, &args.temp.?) catch unreachable;

    // args.temp.?.set(choice) catch unreachable;
    // args.result.?.add(&args.result.?, &args.temp.?) catch unreachable;

    // args.temp.?.set(1) catch unreachable;
    // args.result.?.add(&args.result.?, &args.temp.?) catch unreachable;

    // return true;
}

pub fn mixedRadixLoadExprToSlice(expr: *LambdaExpression, allocator: std.mem.Allocator) ![]const u8 {
    //
    var args: MixedRadixArgs = .{ .allocator = allocator };
    args.init();
    defer args.deinit();

    expr.traversePost(mixedRadixTraverseFn, &args, 0) catch unreachable;

    //AI CODE
    // Subtract 1 so `/(1)` (result = 2) becomes `1` ("1"[1..] == "")
    args.temp.?.set(1) catch unreachable;
    try args.result.?.sub(&args.result.?, &args.temp.?);

    return (try args.result.?.toString(allocator, 2, std.fmt.Case.lower))[1..];
}

// def decode(bits):
//     # Reverse step 3: binary string -> integer N
//     n = int('1' + bits, 2)

//     # Reverse step 2 & 1: unpack choices and rebuild tree
//     def build(d=0):
//         nonlocal n
//         menu = d + 2
//         n -= 1
//         choice = n % menu
//         n //= menu

//         if choice == 0:
//             return ('abs', build(d + 1))
//         elif choice == 1:
//             return ('app', build(d), build(d))
//         else:
//             return ('var', choice - 1)
//     return build()
// Use code with caution.
// Notice a neat trick in the code: since step 2 ends with N - 1 and step 3 immediately does N + 1, they cancel out—so you don't even have to subtract and re-add 1!
// Would you like to trace through how decode("0101") unpacks back into λf. λx. x step by step, or see how this handles open terms (free variables)?
