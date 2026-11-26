const std = @import("std");

const LambdaParser = @import("LambdaParser.zig").LambdaParser;
const LambdaInterpreter = @import("LambdaInterpreter.zig").LambdaInterpreter;

const LambdaBuilder = @import("LambdaBuilder.zig").LambdaBuilder;

const JotParser = @import("JotParser.zig").JotParser;
const JotParserMake = @import("JotParser.zig").JotParserMake;
const blcPrintExpression = @import("Blc.zig").blcPrintExpression;
const loadBlcExpressionToSlice = @import("Blc.zig").loadBlcExpressionToSlice;

const mixedRadixLoadExprToSlice = @import("MixedRadixTerms.zig").mixedRadixLoadExprToSlice;
const ArithmeticCoder = @import("BigIntArithmeticCoding.zig").BigIntRangeCoder;

const BigFloat = @import("BFP").BigFloat(.{ .Significand = f64, .Exponent = i64 });
const BigIntManaged = std.math.big.int.Managed;

const BFGenetic = @import("BFVectorGeneticEvolution.zig");

const T3 = @This();

const debugPrintON = false;
fn debugPrint(comptime fmt: []const u8, args: anytype) void {
    if (debugPrintON) std.debug.print(fmt, args);
}

pub const SizeAndOutput = struct { inputSize: u32, output: []const u8 };

pub const SizeTypeDivTen = enum {
    MINUS_TWENTYONE, //
    MINUS_THIRTEEN,
    MINUS_FIVE,
    MINUS_SEVENTEEN,
    MINUS_NINE,
};

pub fn sizeGetTypeDivTen(size: u32) ?SizeTypeDivTen {
    if (size >= 21 and (size - 21) % 10 == 0) return SizeTypeDivTen.MINUS_TWENTYONE;
    if (size >= 13 and (size - 13) % 10 == 0) return SizeTypeDivTen.MINUS_THIRTEEN;
    if (size >= 5 and (size - 5) % 10 == 0) return SizeTypeDivTen.MINUS_FIVE;
    if (size >= 17 and (size - 17) % 10 == 0) return SizeTypeDivTen.MINUS_SEVENTEEN;
    if (size >= 9 and (size - 9) % 10 == 0) return SizeTypeDivTen.MINUS_NINE;
    return null;
}

pub const SizeTypeDivFour = enum {
    MINUS_TWENTYONE, //
    MINUS_THIRTEEN,
};
pub fn sizeGetTypeDivFour(size: u32) ?SizeTypeDivFour {
    if (size >= 21 and (size - 21) % 4 == 0) return SizeTypeDivFour.MINUS_TWENTYONE;
    if (size >= 13 and (size - 13) % 4 == 0) return SizeTypeDivFour.MINUS_THIRTEEN;
    return null;
}

/// L is the last considered maxTraverses from the sampling phase.
/// b is the fitted parameter for the exponent in the exponential model in the fitting phase.
/// M is the maxTraverses value at which we capture 99.99999997% of halting machines (a good estimate)
pub fn getStepsCapture5Sigma(b: f64, L: f64) BigFloat {
    const e: BigFloat = .init(std.math.e);
    const twoSevenNum: BigFloat = .init(2.71828);
    const bTimesL: BigFloat = .init(b * L);
    const lottaSix: BigFloat = .init(6666666113);
    const oneFive: BigFloat = .init(1.5e-10);
    const bigB: BigFloat = .init(b);

    // M = -( ln( 1.5E-10 * (2.71828 ^(b*L) + 6666666113)) - b*L ) / (b)
    // M = (1.5E-10.mul(2.71828.pow(b*L).add(6666666113))).sub(b*L)).log2().div(e.log2()).div(b).neg()

    const M: BigFloat = oneFive.mul(twoSevenNum.pow(bTimesL).add(lottaSix)).log2().div(e.log2()).sub(bTimesL).div(bigB).neg();

    std.debug.print("\n {} exponent, {} steps -> {f} steps to capture\n", .{ b, L, M });

    return M;
}

//TODO revisit this to make it more accurate or something
// a=0.241582 b=1.01114 c=9.28113, maxProgs = a*b^(c*size)
// maxProgsEstimate = 0.241582 * 1.01114 ^ (9.28113 * size)
// pub fn maxEstimatedProgramsOfSize(size: usize) !BigFloat {
//     const oneThing = BigFloat.init(1.01114);
//     const nineThing = BigFloat.init(9.28113);
//     const sizeThing = BigFloat.init(size);
//     const pointTwo = BigFloat.init(0.241582);

//     return oneThing.pow(nineThing.mul(sizeThing)).mul(pointTwo);
// }

fn firstIndexSeenFuncThing(r: *BigIntManaged, x: *const BigIntManaged, a: *const BigIntManaged, b: *const BigIntManaged, c: *const BigIntManaged, allocator: std.mem.Allocator) !void { //
    var temp1 = try BigIntManaged.init(allocator);
    var temp2 = try BigIntManaged.init(allocator);
    var temp3 = try BigIntManaged.init(allocator);
    defer temp1.deinit();
    defer temp2.deinit();
    defer temp3.deinit();

    try temp2.set(10);
    try temp1.sub(x, a);
    try temp1.divFloor(&temp3, &temp1, &temp2);
    try temp1.add(&temp1, b);

    try temp2.set(2);
    try temp1.pow(&temp2, try temp1.toInt(u32));

    try r.add(&temp1, c);
}

pub fn firstIndexSeenForProgramSize(r: *BigIntManaged, size: *const BigIntManaged, allocator: std.mem.Allocator) !bool { //
    var temp1 = try BigIntManaged.init(allocator);
    var temp2 = try BigIntManaged.init(allocator);
    var temp3 = try BigIntManaged.init(allocator);
    var temp4 = try BigIntManaged.init(allocator);
    defer temp1.deinit();
    defer temp2.deinit();
    defer temp3.deinit();
    defer temp4.deinit();

    try temp2.set(5);
    // try temp1.sub(size, &temp2);
    // try temp4.set(10);
    // try temp1.divFloor(&temp3, &temp1, &temp4);
    const sizeInt = try size.toInt(u32);
    if (sizeInt >= 5 and (sizeInt - 5) % 10 == 0) {
        try temp1.set(0);
        try temp3.set(0);
        try firstIndexSeenFuncThing(r, size, &temp2, &temp1, &temp3, allocator);
        return true;
    }

    try temp2.set(9);
    // try temp1.sub(size, &temp2);
    // try temp4.set(10);
    // try temp1.divFloor(&temp3, &temp1, &temp4);
    if (sizeInt >= 9 and (sizeInt - 9) % 10 == 0) {
        try temp1.set(0);
        try temp3.set(1);
        try firstIndexSeenFuncThing(r, size, &temp2, &temp1, &temp3, allocator);
        return true;
    }

    try temp2.set(13);
    // try temp1.sub(size, &temp2);
    // try temp4.set(10);
    // try temp1.divFloor(&temp3, &temp1, &temp4);
    if (sizeInt >= 13 and (sizeInt - 13) % 10 == 0) {
        try temp1.set(2);
        try temp3.set(3);
        try firstIndexSeenFuncThing(r, size, &temp2, &temp1, &temp3, allocator);
        return true;
    }

    try temp2.set(17);
    // try temp1.sub(size, &temp2);
    // try temp4.set(10);
    // try temp1.divFloor(&temp3, &temp1, &temp4);
    if (sizeInt >= 17 and (sizeInt - 17) % 10 == 0) {
        try temp1.set(3);
        try temp3.set(7);
        try firstIndexSeenFuncThing(r, size, &temp2, &temp1, &temp3, allocator);
        return true;
    }

    try temp2.set(21);
    // try temp1.sub(size, &temp2);
    // try temp4.set(10);
    // try temp1.divFloor(&temp3, &temp1, &temp4);
    if (sizeInt >= 21 and (sizeInt - 21) % 10 == 0) {
        try temp1.set(4);
        try temp3.set(15);
        try firstIndexSeenFuncThing(r, size, &temp2, &temp1, &temp3, allocator);
        return true;
    }
    return false;
}

fn lastIndexSeenFuncThing(r: *BigIntManaged, x: *const BigIntManaged, a: *const BigIntManaged, b: *const BigIntManaged, c: *const BigIntManaged, allocator: std.mem.Allocator) !void { //
    var temp1 = try BigIntManaged.init(allocator);
    var temp2 = try BigIntManaged.init(allocator);
    var temp3 = try BigIntManaged.init(allocator);
    defer temp1.deinit();
    defer temp2.deinit();
    defer temp3.deinit();

    try temp2.set(4);
    try temp1.sub(x, a);
    try temp1.divFloor(&temp3, &temp1, &temp2);
    try temp1.add(&temp1, b);

    try temp2.set(2);
    try temp3.pow(&temp2, try temp1.toInt(u32));

    try r.sub(&temp3, c);
}

pub fn lastIndexSeenForProgramSize(r: *BigIntManaged, size: *const BigIntManaged, allocator: std.mem.Allocator) !bool { //
    var temp1 = try BigIntManaged.init(allocator);
    var temp2 = try BigIntManaged.init(allocator);
    var temp3 = try BigIntManaged.init(allocator);
    var temp4 = try BigIntManaged.init(allocator);
    defer temp1.deinit();
    defer temp2.deinit();
    defer temp3.deinit();
    defer temp4.deinit();

    const sizeInt = try size.toInt(i32);

    try temp2.set(17);
    // try temp1.sub(size, &temp2);
    // try temp4.set(4);
    // try temp1.divFloor(&temp3, &temp1, &temp4);
    if (sizeInt >= 5 and @abs(sizeInt - 17) % 4 == 0) {
        try temp1.set(4);
        try temp3.set(1);
        try lastIndexSeenFuncThing(r, size, &temp2, &temp1, &temp3, allocator);
        return true;
    }

    try temp2.set(23);
    // try temp1.sub(size, &temp2);
    // try temp4.set(4);
    // try temp1.divFloor(&temp3, &temp1, &temp4);
    if (sizeInt >= 9 and @abs(sizeInt - 23) % 4 == 0) {
        try temp1.set(4);
        try temp3.set(2);
        try lastIndexSeenFuncThing(r, size, &temp2, &temp1, &temp3, allocator);
        return true;
    }
    return false;
}

pub fn getSize(binarySlice: []const u8, allocator: std.mem.Allocator) !u32 {
    var jotParser: JotParser = JotParserMake(allocator);
    defer jotParser.deinit();

    debugPrint("\t\"{s}\"\n\t=jotParse> ", .{binarySlice});
    const jotExpr = try jotParser.parse(binarySlice);
    if (debugPrintON) jotExpr.traversePrint();
    debugPrint("\n", .{});
    return jotExpr.size();
}

pub fn streamBinaryStrsRange(
    // allocator: std.mem.Allocator, //
    a: BigIntManaged,
    b: BigIntManaged,
    comptime foreach: *const fn (*BigIntManaged, []const u8, args: anytype) error{ Yield, Else }!void,
    args: anytype,
    progressPrint: bool,
) !void {
    var fmtBuffer: [1024]u8 = undefined;

    var i: BigIntManaged = try a.clone();
    defer i.deinit();

    var progressI: BigIntManaged = try b.clone();
    defer progressI.deinit();

    var r: BigIntManaged = try b.clone();
    defer r.deinit();

    var ten: BigIntManaged = try b.clone();
    defer ten.deinit();
    try ten.set(10);

    if (progressPrint) {
        try progressI.divFloor(&r, &b, &ten);
        try r.copy(progressI.toConst());
    }

    var onwards = true;
    var percent: u32 = 10;
    while (onwards) {
        if (progressPrint and progressI.eql(i)) {
            try progressI.add(&progressI, &r);
            std.debug.print("{d}% reached\n", .{percent});
            percent += 10;
        }
        @memset(&fmtBuffer, 0);

        const binarySlice = try std.fmt.bufPrint(&fmtBuffer, "{b}", .{i});

        // std.debug.print("\n{s}", .{binarySlice});

        try foreach(&i, binarySlice, args);

        onwards = switch (i.order(b)) {
            std.math.Order.eq => false,
            std.math.Order.gt => false,
            std.math.Order.lt => true,
        };
        if (onwards) {
            try i.addScalar(&i, 1);
        }
    }
}

pub fn randomBigIntRange(r: *BigIntManaged, a: *const BigIntManaged, b: *const BigIntManaged, rng: *std.Random.DefaultPrng) !void {
    // std.debug.assert(a.order(b) != .gt); // Ensure a <= b

    var rangeLength = try a.clone();
    var temp = try a.clone();
    var temp2 = try a.clone();
    var rngMaxValue = try a.clone();

    defer {
        rangeLength.deinit();
        temp.deinit();
        temp2.deinit();
        rngMaxValue.deinit();
    }

    try rangeLength.sub(b, a);
    rangeLength.abs();

    const rangeBits: u32 = @intCast(rangeLength.bitCountTwosComp());

    const rangeLength_U64s: u32 =
        if (rangeBits % 64 != 0) rangeBits / 64 + 1 else rangeBits / 64;

    try temp.set(2);
    try rngMaxValue.pow(&temp, rangeLength_U64s * 64);
    // try temp.set(1);
    try rngMaxValue.addScalar(&rngMaxValue, -1);

    //load random u64's
    try r.set(0);
    for (0..rangeLength_U64s) |_| {
        try r.shiftLeft(r, 64); // make space for the next u64

        try temp.set(rng.next());
        try r.bitOr(r, &temp); //load the random bytes
    }

    //scale it down to match the rangeLength
    try temp.divFloor(&temp2, &rngMaxValue, &rangeLength);
    try r.divFloor(&temp2, r, &temp);

    //add a to keep it in range
    try r.add(r, a);
    // if (!a.isPositive()) {
    //     try temp.set(1);
    //     try r.add(r, &temp);
    // }
}

pub const FeistelCipher = struct {
    const Self = @This();

    const rounds = 4;

    allocator: std.mem.Allocator = undefined,
    state: BigIntManaged = undefined,
    leftIndex: BigIntManaged = undefined, // left index expected by user
    // rightIndex: BigIntManaged = undefined,
    lastIndex: BigIntManaged = undefined, // last index we check when cycle walking
    bits: u32 = 0,

    randomSeed: u64 = 42,

    roundLeft: BigIntManaged = undefined,
    roundRight: BigIntManaged = undefined,
    roundF: BigIntManaged = undefined,
    roundR: BigIntManaged = undefined,
    roundLeftBitmask: BigIntManaged = undefined,
    roundRightBitmask: BigIntManaged = undefined,

    roundMsg: BigIntManaged = undefined,

    index: BigIntManaged = undefined,

    superSmallMode: bool = false,
    smallArray: []u32,
    smallIndex: u32 = 0,

    pub fn init(
        self: *Self,
        allocator: std.mem.Allocator,
        left: *const BigIntManaged,
        right: *const BigIntManaged,
        randomSeed: u64,
    ) !void {
        self.allocator = allocator;

        // self.rightIndex = try BigIntManaged.cloneWithDifferentAllocator(right.*, allocator);
        self.lastIndex = try BigIntManaged.init(allocator);
        self.leftIndex = try BigIntManaged.cloneWithDifferentAllocator(left.*, allocator);

        try self.lastIndex.sub(right, left);
        self.lastIndex.abs();

        const smallModeThresholdValU32: u32 = 5000;
        var smallThresholdVal = try BigIntManaged.initSet(allocator, //
            smallModeThresholdValU32);
        defer smallThresholdVal.deinit();

        if (self.lastIndex.order(smallThresholdVal) == .lt) {
            self.superSmallMode = true;

            const arraySize: u32 = try self.lastIndex.toInt(u32);

            self.smallArray = try allocator.alloc(u32, arraySize);

            for (0..arraySize) |i| {
                const iU32: u32 = @intCast(i);
                self.smallArray[i] = iU32;
            }

            var rand = std.Random.DefaultPrng.init(randomSeed);
            std.Random.shuffle(rand.random(), u32, self.smallArray);
        } else {
            self.state = try BigIntManaged.initSet(allocator, 0);
            try self.lastIndex.addScalar(&self.lastIndex, -1);
            const rangeBits: u32 = @intCast(self.lastIndex.bitCountTwosComp());

            const bits = if (rangeBits % 2 == 0) rangeBits else rangeBits + 1;
            self.bits = bits;

            self.randomSeed = randomSeed;

            self.roundLeft = try BigIntManaged.init(allocator);
            self.roundRight = try BigIntManaged.init(allocator);
            self.roundR = try BigIntManaged.init(allocator);
            self.roundF = try BigIntManaged.init(allocator);
            self.roundLeftBitmask = try BigIntManaged.init(allocator);
            self.roundRightBitmask = try BigIntManaged.init(allocator);
            self.roundMsg = try BigIntManaged.init(allocator);
            self.index = try BigIntManaged.initSet(allocator, 0);

            try self.roundRightBitmask.set(2);
            try self.roundRightBitmask.pow(&self.roundRightBitmask, self.bits / 2);
            try self.roundRightBitmask.addScalar(&self.roundRightBitmask, -1);

            try self.roundLeftBitmask.copy(self.roundRightBitmask.toConst());
            try self.roundLeftBitmask.shiftLeft(&self.roundLeftBitmask, self.bits / 2);
        }

        // var temp = try BigIntManaged.cloneWithDifferentAllocator(self.lastIndex, allocator);
        // defer {
        //     temp.deinit();
        // }

        // try temp.addScalar(&temp, -1); //might need to delete this line

        // std.debug.print("left: {d}, right: {d}, bits: {}, leftMask: {d}, rightMask: {d}, lastIndex: {d}\n", .{ left.toConst(), right.toConst(), self.bits, self.roundLeftBitmask, self.roundRightBitmask, self.lastIndex });
    }

    pub fn deinit(self: *Self) void {
        self.lastIndex.deinit();
        self.leftIndex.deinit();
        if (self.superSmallMode) {
            self.allocator.free(self.smallArray);
        } else {
            self.state.deinit();
            self.roundLeft.deinit();
            self.roundRight.deinit();
            self.roundF.deinit();
            self.roundR.deinit();
            self.roundLeftBitmask.deinit();
            self.roundRightBitmask.deinit();
            self.roundMsg.deinit();
            self.index.deinit();
        }
    }

    fn getNRandomBits(r: *BigIntManaged, msg: *const BigIntManaged, N: u32, allocator: std.mem.Allocator) !void {
        _ = allocator;
        //create rng from msg
        const seed = std.hash.Wyhash.hash(0, std.mem.sliceAsBytes(msg.limbs[0..msg.len()]));
        var rng = std.Random.DefaultPrng.init(seed);
        //load random u64s
        const leftoverBits =
            if (N > 64)
                N % 64
            else if (N < 64)
                64 - N
            else
                0;
        const u64Loads: u32 =
            if (N > 64)
                if (leftoverBits > 0)
                    (N / 64 + 1)
                else
                    N / 64
            else
                1;

        try r.set(0);
        for (0..u64Loads) |_| {
            try r.shiftLeft(r, 64);
            try r.addScalar(r, rng.next());
        }
        //trim to match desired bits
        if (leftoverBits > 0) {
            try r.shiftRight(r, leftoverBits);
        }
        // std.debug.print("u64loads: {}, leftoverBits: {}\n", .{ u64Loads, leftoverBits });
    }

    fn round(
        self: *Self,
    ) !void {
        // msg = [R,Right]
        try self.roundMsg.set(0);

        //AI CODE
        try self.roundMsg.set(self.randomSeed);
        try self.roundMsg.shiftLeft(&self.roundMsg, 32);
        try self.roundMsg.bitOr(&self.roundMsg, &self.roundR);
        try self.roundMsg.shiftLeft(&self.roundMsg, self.bits / 2);
        try self.roundMsg.bitOr(&self.roundMsg, &self.roundRight);

        //my code
        // try self.roundMsg.bitOr(&self.roundMsg, &self.roundR);
        // try self.roundMsg.shiftLeft(&self.roundMsg, self.roundR.bitCountTwosComp());
        // try self.roundMsg.bitOr(&self.roundMsg, &self.roundRight);

        // std.debug.print("msg {d}\n", .{self.roundMsg});

        // f = getNRandomBits(msg, N=self.bits/2)
        try getNRandomBits(&self.roundF, &self.roundMsg, self.bits / 2, self.allocator);
    }

    fn encrypt(
        self: *Self,
    ) !void {
        // left = (leftMask & state) >> (bits/2)
        try self.roundLeft.bitAnd(&self.roundLeftBitmask, &self.state);
        try self.roundLeft.shiftRight(&self.roundLeft, self.bits / 2);
        // right = rightMask & state
        try self.roundRight.bitAnd(&self.roundRightBitmask, &self.state);

        for (0..rounds) |r| {
            // f = round(r, right)
            try self.roundR.set(r);
            try self.round();
            // try self.roundF.bitAnd(&self.roundF, &self.roundRightBitmask);

            // std.debug.print("state: {d}, f: {d}\n ", .{ self.state, self.roundF });

            //AI CODE
            // Compute newRight = oldLeft ^ f BEFORE overwriting roundLeft
            try self.roundF.bitXor(&self.roundLeft, &self.roundF); // reuse roundF to hold newRight
            try self.roundLeft.copy(self.roundRight.toConst()); // left = oldRight
            try self.roundRight.copy(self.roundF.toConst()); // right = newRight

            // // right = left ^ f
            // try self.roundRight.bitXor(&self.roundLeft, &self.roundF);
            // // left = right
            // try self.roundLeft.copy(self.roundRight.toConst());

            // std.debug.print("left: {d}, right {d}\n", .{ self.roundLeft, self.roundRight });
        }
        //state = (left << half bits) | right
        try self.roundLeft.shiftLeft(&self.roundLeft, self.bits / 2);
        try self.state.bitOr(&self.roundLeft, &self.roundRight);

        // std.debug.print("{d}\n", .{self.state.toConst()});
    }

    pub fn loadNext(
        self: *Self,
        r: *BigIntManaged,
    ) !void {
        if (self.superSmallMode) {
            if (self.smallIndex >= self.smallArray.len) self.smallIndex = 0;

            const value: u32 = if (self.smallArray.len == 0) 0 else self.smallArray[self.smallIndex];

            try r.set(value);
            try r.add(r, &self.leftIndex);

            self.smallIndex += 1;
        } else {
            //make sure index stays under the range so it doesnt break
            if (self.index.order(self.lastIndex) == .gt) {
                try self.index.set(0);
            }

            //cycle walk
            try self.state.copy(self.index.toConst());
            try self.encrypt();
            // const lastIndexConst = self.lastIndex.toConst();
            while (self.state.order(self.lastIndex) == .gt) {
                try self.encrypt();
            }

            try r.copy(self.state.toConst());
            try r.add(r, &self.leftIndex);

            try self.index.addScalar(&self.index, 1);
        }
    }
};

// fn percentGetDigit(float: f64, digit: u32) u32 {
//     return @trunc(float * @as(f64, @floatFromInt((10 ^ digit + 1))));
// }

/// the precision is limited by f64 type so beware
pub fn bigIntPercent(r: *BigIntManaged, bigInt: *const BigIntManaged, precision: u32, percent: f64, allocator: std.mem.Allocator) !void {
    var temp1 = try BigIntManaged.initSet(allocator, 0);
    var temp2 = try BigIntManaged.init(allocator);
    var temp3 = try BigIntManaged.init(allocator);

    defer {
        temp1.deinit();
        temp2.deinit();
        temp3.deinit();
    }

    const bigAhhInt: u32 = std.math.pow(u32, 10, precision);
    const percentIntilized: u32 = @trunc(percent * @as(f64, @floatFromInt(bigAhhInt)));

    try temp1.set(percentIntilized);
    try temp1.mul(bigInt, &temp1);

    try temp2.set(bigAhhInt);

    // std.debug.print("div {d} by {d}\n", .{ temp1, temp2 });

    try r.divFloor(&temp3, &temp1, &temp2);
}

pub fn streamUniqueRandomBinaryStrsRange(
    allocator: std.mem.Allocator, //
    a: BigIntManaged,
    b: BigIntManaged,
    percentFromRange: f64,
    randomSeed: u32,
    comptime foreach: *const fn (*BigIntManaged, []const u8, args: anytype) error{ Yield, Else }!void,
    args: anytype,
    progressPrint: bool,
) !void {
    var fmtBuffer: [1024]u8 = undefined;

    var i: BigIntManaged = try a.clone();

    var progressI: BigIntManaged = try b.clone();

    var r: BigIntManaged = try b.clone();

    var ten: BigIntManaged = try b.clone();
    try ten.set(10);

    var indexCounter = try BigIntManaged.init(allocator);
    try indexCounter.sub(&b, &a);
    try bigIntPercent(&indexCounter, &indexCounter, 4, percentFromRange, allocator);

    var rng: T3.FeistelCipher = undefined;
    // std.debug.print("before cipher init\n", .{});
    try rng.init(allocator, &a, &b, randomSeed);
    // std.debug.print("after cipher init\n", .{});

    defer {
        i.deinit();
        progressI.deinit();
        r.deinit();
        ten.deinit();
        indexCounter.deinit();
        rng.deinit();
    }

    // std.debug.print("before progressPrint\n", .{});
    if (progressPrint) {
        try progressI.divFloor(&r, &indexCounter, &ten);
        try r.copy(progressI.toConst());
    }

    var onwards = true;
    var percent: u32 = 10;
    // std.debug.print("before onwards while loop\n", .{});
    while (onwards) {
        // std.debug.print("before progress print check\n", .{});
        if (progressPrint and progressI.eql(i)) {
            // std.debug.print("progress is about to print\n", .{});
            try progressI.add(&progressI, &r);
            // std.debug.print("{d}% reached\n", .{percent});
            percent += 10;
        }
        @memset(&fmtBuffer, 0);

        // std.debug.print("before loading cipher next index\n", .{});
        if (onwards) {
            // std.debug.print("about to load next index\n", .{});
            try indexCounter.addScalar(&indexCounter, -1);
            try rng.loadNext(&i);
            // std.debug.print("after loaded next cipher rng\n", .{});
        }

        const binarySlice = try std.fmt.bufPrint(&fmtBuffer, "{b}", .{i});

        // std.debug.print("trying {s}\n", .{binarySlice});

        try foreach(&i, binarySlice, args);

        try i.set(0);
        onwards = switch (indexCounter.order(i)) {
            .eq => false,
            .gt => true,
            .lt => false,
        };
    }
}

// const exp6HashmapKey = struct {
//     size: u32, //
//     steps: u32,
// };
//map<size -> map<steps -> haltedCount >>
const exp6stepHaltedHashmapType = std.AutoHashMap(u32, u32);
const exp6StepsHashmapType = std.AutoHashMap(u32, exp6stepHaltedHashmapType);

const exp6SizeTypeSubHashmap = std.AutoHashMap(u32, u32);
const exp6SizeTypeHashmap = std.AutoHashMap(SizeTypeDivTen, *exp6SizeTypeSubHashmap);

const exp6StepModeArgs = struct {
    const Self = @This();

    hashmap: *exp6StepsHashmapType,
    totalsHashmap: *exp6StepsHashmapType, // size -> <steps -> total>
    haltedStepHashmap: *std.AutoHashMap(u32, u32),

    // this is currently not used
    sizeTypedHashmap: *exp6SizeTypeHashmap, // sizeType -> (size->haltingStep)

    fn doIfHalted(
        stepsArgs: *const Self, //
        // modeArgs: exp6modeArgs,
        size: u32,
        traverses: u32,
        allocator: std.mem.Allocator,
    ) !void {
        // modeArgs.haltedStepHashmap.put(size, traverses) catch return error.Else;
        stepsArgs.haltedStepHashmap.put(size, traverses) catch return error.Else;

        // const sizeTypeDivTenMaybe = sizeGetTypeDivTen(size);

        // if (sizeTypeDivTenMaybe) |sizeTypeDivTen| {
        //     const sizeTypedHashmapMaybe = stepsArgs.sizeTypedHashmap.get(sizeTypeDivTen);
        //     if (sizeTypedHashmapMaybe) |sizeTypedHashmap| {
        //         sizeTypedHashmap.put(size, traverses) catch return error.Else;
        //     } else {
        //         const sizeTypedHashmap = allocator.create(exp6SizeTypeSubHashmap) catch return error.Else;
        //         sizeTypedHashmap.* = exp6SizeTypeSubHashmap.init(allocator);

        //         stepsArgs.sizeTypedHashmap.put(sizeTypeDivTen, sizeTypedHashmap) catch return error.Else;

        //         sizeTypedHashmap.put(size, traverses) catch return error.Else;
        //     }
        // }

        const thing = stepsArgs.hashmap.getOrPut(size) catch return error.Else;
        if (!thing.found_existing) {
            thing.value_ptr.* = exp6stepHaltedHashmapType.init(allocator);
        }

        const haltedCountRes = thing.value_ptr.*.getOrPut(traverses) catch return error.Else;
        if (haltedCountRes.found_existing) {
            haltedCountRes.value_ptr.* += 1;
        } else {
            haltedCountRes.value_ptr.* = 1;
        }
    }

    fn doAfterHalting(
        stepsArgs: *const Self, //
        size: u32,
        traverses: u32,
        allocator: std.mem.Allocator,
    ) !void {
        const thing = stepsArgs.totalsHashmap.getOrPut(size) catch return error.Else;
        if (!thing.found_existing) {
            thing.value_ptr.* = exp6stepHaltedHashmapType.init(allocator);
        }

        const totalRes = thing.value_ptr.*.getOrPut(traverses) catch return error.Else;
        if (totalRes.found_existing) {
            totalRes.value_ptr.* += 1;
        } else {
            totalRes.value_ptr.* = 1;
        }
    }
};

//map<size -> map< (steps,mem) -> haltedCount >>
const StepAndMem = struct { step: u32, mem: u32 };
const exp6StepAndMemCountHashmap = std.AutoHashMap(StepAndMem, u32);
const exp6memCountHashmap = std.AutoHashMap(u32, exp6StepAndMemCountHashmap);

const exp6MemoryModeArgs = struct {
    const Self = @This();

    countHashmap: *exp6memCountHashmap,
    totalsHashmap: *exp6memCountHashmap,
    // haltedMemHashmap: void,

    fn doIfHalted(
        memArgs: *const Self, //
        size: u32,
        traverses: u32,
        chosenMem: u32,
        allocator: std.mem.Allocator,
    ) !void {
        // modeArgs.haltedStepHashmap.put(size, traverses) catch return error.Else;
        // stepsArgs.haltedStepHashmap.put(size, traverses) catch return error.Else;
        // const sizeTypeDivTenMaybe = sizeGetTypeDivTen(size);
        // if (sizeTypeDivTenMaybe) |sizeTypeDivTen| {
        //     const sizeTypedHashmapMaybe = memArgs.sizeTypedHashmap.get(sizeTypeDivTen);
        //     if (sizeTypedHashmapMaybe) |sizeTypedHashmap| {
        //         sizeTypedHashmap.put(size, traverses) catch return error.Else;
        //     } else {
        //         const sizeTypedHashmap = memArgs.allocator.create(exp6SizeTypeSubHashmap) catch return error.Else;
        //         sizeTypedHashmap.* = exp6SizeTypeSubHashmap.init(memArgs.allocator);

        //         memArgs.sizeTypedHashmap.put(sizeTypeDivTen, sizeTypedHashmap) catch return error.Else;

        //         sizeTypedHashmap.put(size, traverses) catch return error.Else;
        //     }
        // }

        const thing = memArgs.countHashmap.getOrPut(size) catch return error.Else;
        if (!thing.found_existing) {
            thing.value_ptr.* = exp6StepAndMemCountHashmap.init(allocator);
        }

        const stepAndMemKey: StepAndMem = .{ .step = traverses, .mem = chosenMem };
        const haltedCountRes = thing.value_ptr.*.getOrPut(stepAndMemKey) catch return error.Else;
        if (haltedCountRes.found_existing) {
            haltedCountRes.value_ptr.* += 1;
        } else {
            haltedCountRes.value_ptr.* = 1;
        }
    }

    fn doAfterHalting(
        memArgs: *const Self, //
        size: u32,
        traverses: u32,
        chosenMem: u32,
        allocator: std.mem.Allocator,
    ) !void {
        const thing = memArgs.totalsHashmap.getOrPut(size) catch return error.Else;
        if (!thing.found_existing) {
            thing.value_ptr.* = exp6StepAndMemCountHashmap.init(allocator);
        }

        const stepAndMemKey: StepAndMem = .{ .step = traverses, .mem = chosenMem };
        const totalRes = thing.value_ptr.*.getOrPut(stepAndMemKey) catch return error.Else;
        if (totalRes.found_existing) {
            totalRes.value_ptr.* += 1;
        } else {
            totalRes.value_ptr.* = 1;
        }
    }
};

const exp6modeArgs = union(enum) {
    onlySteps: struct { args: exp6StepModeArgs },
    stepsAndMemory: struct { args: exp6MemoryModeArgs },
    // stepsAndMemory: struct { stepArgs: exp6StepModeArgs, memArgs: exp6MemoryModeArgs },
};

const exp6RandomArgs = struct {
    const Self = @This();

    prng: *std.Random.DefaultPrng,
    percentIgnore: f64,
    sizeStartIgnore: ?u32 = null,

    fn shouldReturn(randomArgs: *const Self, size: u32) bool {
        // if (randomArgs.percentIgnore != null and randomArgs.prng != null) {
        const random = randomArgs.prng.random().float(f64);
        if (randomArgs.sizeStartIgnore != null) {
            if (randomArgs.sizeStartIgnore.? <= size and random <= randomArgs.percentIgnore) return true;
        } else {
            if (random <= randomArgs.percentIgnore) return true;
        }
        // }
        return false;
    }
};
const exp6args = struct {
    allocator: std.mem.Allocator, //
    interpreterAllocator: *std.heap.FixedBufferAllocator,

    chosenMemory: u32,
    chosenSize: ?u32,
    chosenSteps: u32,
    yieldOnFirstHalt: bool = false,

    randomArgs: ?exp6RandomArgs,

    modeArgs: exp6modeArgs,
};
fn exp6(_: *BigIntManaged, input: []const u8, argz: anytype) error{ Yield, Else }!void {
    const args: exp6args = argz;

    const size = getSize(input, args.allocator) catch return error.Else;

    if (args.chosenSize) |chosenSize| if (size != chosenSize) {
        // std.debug.print("sizes different? {} vs {}\n", .{ size, chosenSize });
        return;
    };

    // std.debug.print("computing a size {}\n", .{size});

    if (args.randomArgs) |rArgs| if (rArgs.shouldReturn(size)) return;

    args.interpreterAllocator.reset();
    // var t3Arena = std.heap.ArenaAllocator.init(args.allocator);
    // defer t3Arena.deinit();

    const traverses = args.chosenSteps;

    // std.debug.print("before computation.\n", .{});
    // for (1..args.maxSteps) |traverses| {
    const machineResult = cacaHalted(input, traverses, args.interpreterAllocator.allocator());

    // std.debug.print("after computation.\n", .{});

    const halted = if (machineResult) |didItHalt|
        didItHalt
    else |err| if (err == error.OutOfMemory) oom: {
        // std.debug.print("ran out of memory!\n", .{});
        break :oom false;
    } else {
        std.debug.print("else!\n", .{});
        return error.Else;
    };

    // if it halted, store some data about it
    if (halted) {
        // std.debug.print("halted, doing stuff\n", .{});
        // std.debug.print("halted!\n", .{});
        switch (args.modeArgs) {
            .onlySteps => |stepArgsOuter| {
                const stepArgs = stepArgsOuter.args;

                try stepArgs.doIfHalted(size, traverses, args.allocator);
            },
            .stepsAndMemory => |memOuterArgs| {
                const memArgs = memOuterArgs.args;

                try memArgs.doIfHalted(size, traverses, args.chosenMemory, args.allocator);
                // std.debug.print("after if halted\n", .{});
            },
            // .stepsAndMemory => |stepMemOuterArgs| {
            //     const stepArgs = stepMemOuterArgs.stepArgs;
            //     const memArgs = stepMemOuterArgs.memArgs;

            //     stepArgs.doIfHalted(size, traverses);
            //     memArgs.doIfHalted();
            // },
        }
    } else {
        // std.debug.print("did not halt.\n", .{});
    }

    switch (args.modeArgs) {
        .onlySteps => |stepArgsOuter| {
            const stepArgs = stepArgsOuter.args;

            // std.debug.print("before onlySteps after halting\n", .{});
            try stepArgs.doAfterHalting(size, traverses, args.allocator);
            // std.debug.print("after onlySteps after halting\n", .{});
        },
        .stepsAndMemory => |memOuterArgs| {
            const memArgs = memOuterArgs.args;

            // std.debug.print("before memAndStep after halting\n", .{});
            try memArgs.doAfterHalting(size, traverses, args.chosenMemory, args.allocator);
            // std.debug.print("after memAndStep after halting\n", .{});
        },
        // .stepsAndMemory => |stepMemOuterArgs| {
        //     const stepArgs = stepMemOuterArgs.stepArgs;
        //     const memArgs = stepMemOuterArgs.memArgs;

        //     stepArgs.doAfterHalting(size, traverses);
        //     memArgs.doAfterHalting();
        // },
    }

    // const haltedStepMaybe = args.haltedStepHashmap.get(size);
    if (halted and args.yieldOnFirstHalt) return error.Yield;
    // }
}

const binarySearchDebugPrintON = true;
fn binarySearchDebugPrint(comptime fmt: []const u8, args: anytype) void {
    if (binarySearchDebugPrintON) std.debug.print(fmt, args);
}

pub const binarySearchFirstHaltingStepForSizeCache = std.AutoHashMap(u32, u32);

// idk 40 seems good
const minSizeForIgnore = 40;

/// find the first step at which a size halts
pub fn binarySearchFirstHaltingStepForSize(
    sizez: u32, //
    cacheMaybe: ?*binarySearchFirstHaltingStepForSizeCache,
    allocator: std.mem.Allocator,
    interpreterMemoryBytes: u32,
    // interpreterAllocator: *std.heap.FixedBufferAllocator,
    percentIgnoreInput: f64, // percent of machines to skip over/ignore
    randomSeed: u32, //  for reproducability
    hashmapInput: ?*exp6StepsHashmapType,
    totalsMapInput: ?*exp6StepsHashmapType,
) !?u32 {
    const percentIgnore = if (sizez < minSizeForIgnore) 0.0 else percentIgnoreInput;
    if (cacheMaybe) |cache| {
        const cachedRes = cache.get(sizez);
        if (cachedRes) |result| {
            return result;
        }
    }
    // const startSize = 5;
    // const lastSize = 200;
    var first = try BigIntManaged.init(allocator);
    var last = try BigIntManaged.init(allocator);
    var size = try BigIntManaged.init(allocator);

    defer size.deinit();
    defer first.deinit();
    defer last.deinit();

    var hashmapInternal = exp6StepsHashmapType.init(allocator);
    defer hashmapInternal.deinit();

    var totalsMapInternal = exp6StepsHashmapType.init(allocator);
    defer totalsMapInternal.deinit();

    var haltedStepHashmap = std.AutoHashMap(u32, u32).init(allocator);
    defer haltedStepHashmap.deinit();

    var sizeTypedHashmap = exp6SizeTypeHashmap.init(allocator);
    defer sizeTypedHashmap.deinit();

    // for (startSize..lastSize) |chosenSizeUsize| {
    // const chosenSize: u32 = @intCast(chosenSizeUsize);
    const chosenSize: u32 = sizez;

    // std.debug.print("\n", .{});
    try size.set(chosenSize);

    // this only succeeds if the size "exists" because not all numbers correspond to a size
    const indicesSuceed = try firstIndexSeenForProgramSize(&first, &size, allocator) and try lastIndexSeenForProgramSize(&last, &size, allocator);
    try last.addScalar(&last, 1);

    if (indicesSuceed) {
        // std.debug.print("trying size {} -> first {d}, last {d}\n", .{ chosenSize, first, last });

        var currentStep: u32 = 1;

        const interprerBuffer = try allocator.alloc(u8, interpreterMemoryBytes);
        defer allocator.free(interprerBuffer);
        var interpreterAllocator = std.heap.FixedBufferAllocator.init(interprerBuffer);

        var start: u32 = currentStep;
        var end: u32 = currentStep * 2;
        var middle: u32 = ((end - start) / 2) + start;
        var endHalted = false;
        currentStep = end;

        var findingNonZeroStep = true;
        while (findingNonZeroStep) {
            // std.debug.print("first halting: size {}, trying {} steps, S {}, M {}, E {}\n", .{ chosenSize, currentStep, start, middle, end });

            // var prng: std.Random.DefaultPrng = .init(randomSeed orelse 42);

            const modeArgs: exp6modeArgs = .{ .onlySteps = .{ .args = .{
                .hashmap = if (hashmapInput) |the| the else &hashmapInternal,
                .totalsHashmap = if (totalsMapInput) |the| the else &totalsMapInternal,
                .haltedStepHashmap = &haltedStepHashmap,
                .sizeTypedHashmap = &sizeTypedHashmap,
            } } };

            const args: exp6args = .{
                .allocator = allocator, //
                .interpreterAllocator = &interpreterAllocator,
                .chosenSize = chosenSize,
                .chosenSteps = currentStep,
                .chosenMemory = interpreterMemoryBytes,
                .yieldOnFirstHalt = false, // we are only looking for the first halting, dont keep going after finding it

                // .haltedStepHashmap = &haltedStepHashmap,

                .modeArgs = modeArgs,

                // .randomArgs = if (percentIgnore != null) .{
                //     .prng = &prng,
                //     .percentIgnore = percentIgnore,
                //     .sizeStartIgnore = minSizeForSampling, //for safety
                // } else null,
                .randomArgs = null,
            };
            // std.debug.print("before machines thing\n", .{});
            // args.percentIgnore = 0.0;
            streamUniqueRandomBinaryStrsRange(allocator, first, last, 1.0 - percentIgnore, randomSeed, &exp6, args, true) catch |err| {
                if (err == error.Else) {
                    return err;
                }
            };
            // std.debug.print("after machines thing\n", .{});
            // streamBinaryStrsRange(first, last, &exp6, args, false) catch |err| {
            //     if (err == error.Else) {
            //         return err;
            //     }
            // };

            const haltedLastMaybe = haltedStepHashmap.get(chosenSize);
            if (endHalted == false) {
                if (haltedLastMaybe) |_| {
                    endHalted = true;
                    // end = middle;
                    middle = ((end - start) / 2) + start;
                    currentStep = middle;
                } else {
                    end = end * 2;
                    middle = ((end - start) / 2) + start;
                    currentStep = end;
                }
            } else {
                if (haltedLastMaybe) |_| {
                    end = middle;
                } else {
                    start = middle;
                }
                middle = ((end - start) / 2) + start;
                currentStep = middle;
            }

            if (end == start + 1) {
                if (haltedLastMaybe != null) {
                    // std.debug.print("{},{}\n", .{ chosenSize, haltedLastMaybe.? });
                    if (cacheMaybe) |cache| {
                        try cache.put(sizez, haltedLastMaybe.?);
                    }
                    return haltedLastMaybe.?;
                } else {
                    try haltedStepHashmap.put(chosenSize, middle + 1);
                    // std.debug.print("{},{}\n", .{ chosenSize, middle + 1 });
                    if (cacheMaybe) |cache| {
                        try cache.put(sizez, middle + 1);
                    }
                    return middle + 1;
                }
                findingNonZeroStep = false;
            }
            _ = haltedStepHashmap.remove(chosenSize);
        }

        // std.debug.print("found halting greater than 0% at step {}\n", .{currentStep});
    }
    // }
    return null;
}

pub fn sampledBinarySearchNPercentHaltingStepForSize(
    sizez: u32, //
    percentHalted: f64,
    cacheMaybe: ?*binarySearchFirstHaltingStepForSizeCache,
    allocator: std.mem.Allocator,
    interpreterMemoryBytes: u32,
    // interpreterAllocator: *std.heap.FixedBufferAllocator,
    percentIgnoreInput: f64, // percent chance to ignore a machine, e.g 0.95 to ignore ~95% of machines
    randomSeed: u32, //  for reproducability
    hashmapInput: ?*exp6StepsHashmapType,
    totalsMapInput: ?*exp6StepsHashmapType,
) !?u32 {
    const percentIgnore = if (sizez < minSizeForIgnore) 0.0 else percentIgnoreInput;
    if (cacheMaybe) |cache| {
        const cachedRes = cache.get(sizez);
        if (cachedRes) |result| {
            return result;
        }
    }
    // const startSize = 5;
    // const lastSize = 200;
    var first = try BigIntManaged.init(allocator);
    var last = try BigIntManaged.init(allocator);
    var size = try BigIntManaged.init(allocator);

    defer size.deinit();
    defer first.deinit();
    defer last.deinit();

    var hashmapInternal = exp6StepsHashmapType.init(allocator);
    defer hashmapInternal.deinit();

    var totalsMapInternal = exp6StepsHashmapType.init(allocator);
    defer totalsMapInternal.deinit();

    var haltedStepHashmap = std.AutoHashMap(u32, u32).init(allocator);
    defer haltedStepHashmap.deinit();

    var sizeTypedHashmap = exp6SizeTypeHashmap.init(allocator);
    defer sizeTypedHashmap.deinit();

    // for (startSize..lastSize) |chosenSizeUsize| {
    // const chosenSize: u32 = @intCast(chosenSizeUsize);
    const chosenSize: u32 = sizez;

    // std.debug.print("\n", .{});
    try size.set(chosenSize);

    // this only succeeds if the size "exists" because not all numbers correspond to a size
    const indicesSuceed = try firstIndexSeenForProgramSize(&first, &size, allocator) and try lastIndexSeenForProgramSize(&last, &size, allocator);

    try last.addScalar(&last, 1);

    if (indicesSuceed) {
        // std.debug.print("trying size {} -> first {d}, last {d}\n", .{ chosenSize, first, last });

        var currentStep: u32 = 20;

        // dont think this is a good idea
        // if (percentIgnore == null and sizez >= minSizeForGuessing) {
        //     currentStep = try binarySearchNPercentHaltingStepForSize(
        //         sizez, //
        //         percentHalted,
        //         cacheMaybe,
        //         allocator,
        //         0.8,
        //         randomSeed orelse 42,
        //     ) orelse 20;
        // }

        const interprerBuffer = try allocator.alloc(u8, interpreterMemoryBytes);
        defer allocator.free(interprerBuffer);
        var interpreterAllocator = std.heap.FixedBufferAllocator.init(interprerBuffer);

        var start: u32 = currentStep;
        var end: u32 = currentStep * 2;
        var middle: u32 = ((end - start) / 2) + start;
        var end99Halted = false;
        currentStep = end;

        var findingNonZeroStep = true;
        while (findingNonZeroStep) {
            binarySearchDebugPrint("N% halting: size {}, trying {} steps, S {}, M {}, E {}\n", .{ chosenSize, currentStep, start, middle, end });

            // var prng: std.Random.DefaultPrng = .init(randomSeed orelse 42);

            // const interprerBuffer = try allocator.alloc(u8, interpBytesAmount);
            // defer allocator.free(interprerBuffer);
            // var interpreterAllocator = std.heap.FixedBufferAllocator.init(interprerBuffer);

            const modeArgs: exp6modeArgs = .{ .onlySteps = .{ .args = .{
                .hashmap = if (hashmapInput) |the| the else &hashmapInternal,
                .totalsHashmap = if (totalsMapInput) |the| the else &totalsMapInternal,
                .haltedStepHashmap = &haltedStepHashmap,
                .sizeTypedHashmap = &sizeTypedHashmap,
            } } };

            // const randomArgs: exp6RandomArgs = .{
            //     .prng = if (percentIgnore != null) &prng else null,
            //     .percentIgnore = percentIgnore,
            //     .sizeStartIgnore = minSizeForSampling, //for safety
            // };

            var args: exp6args = .{
                .allocator = allocator, //
                .interpreterAllocator = &interpreterAllocator,
                .chosenSize = chosenSize,
                .chosenSteps = currentStep,
                .chosenMemory = interpreterMemoryBytes,

                // .haltedStepHashmap = &haltedStepHashmap,

                .modeArgs = modeArgs,
                // .randomArgs = if (percentIgnore != null) .{
                //     .prng = &prng,
                //     .percentIgnore = percentIgnore,
                //     .sizeStartIgnore = minSizeForSampling, //for safety
                // } else null,
                .randomArgs = null,
            };
            // args.percentIgnore = 0.0;
            streamUniqueRandomBinaryStrsRange(allocator, first, last, 1.0 - percentIgnore, randomSeed, &exp6, args, false) catch |err| {
                if (err == error.Else) {
                    return err;
                }
            };
            // streamBinaryStrsRange(first, last, &exp6, args, false) catch |err| {
            //     if (err == error.Else) {
            //         return err;
            //     }
            // };

            var halted99M: bool = false;

            const totalMapM = args.modeArgs.onlySteps.args.totalsHashmap.get(sizez);
            const haltedMapM = args.modeArgs.onlySteps.args.hashmap.get(sizez);

            if (totalMapM != null and haltedMapM != null) {
                const haltedM = haltedMapM.?.get(currentStep);
                const totalM = totalMapM.?.get(currentStep);
                if (haltedM != null and totalM != null) {
                    const halted: f64 = haltedM.?;
                    const total: f64 = totalM.?;
                    const freqHalted = halted / total;

                    binarySearchDebugPrint("{} halted, {} total, {}% halted\n", .{ halted, total, freqHalted * 100 });

                    const greater99 = freqHalted >= percentHalted;
                    if (greater99) halted99M = true;
                }
            }

            if (end99Halted == false) {
                if (halted99M) {
                    end99Halted = true;
                    // end = middle;
                    middle = ((end - start) / 2) + start;
                    currentStep = middle;
                } else {
                    start = end;
                    end = end * 2;
                    middle = ((end - start) / 2) + start;
                    currentStep = end;
                }
            } else {
                if (halted99M) {
                    end = middle;
                } else {
                    start = middle;
                }
                middle = ((end - start) / 2) + start;
                currentStep = middle;
            }

            if (end == start + 1) {
                if (halted99M) {
                    // const result:StepAndMem = .{.step = currentStep, .mem = interpBytesAmount};
                    const result = currentStep;
                    // std.debug.print("{},{}\n", .{ chosenSize, result });
                    if (cacheMaybe) |cache| {
                        try cache.put(sizez, result);
                    }
                    return result;
                } else {
                    const result = currentStep + 1;
                    // std.debug.print("{},{}\n", .{ chosenSize, result });
                    if (cacheMaybe) |cache| {
                        try cache.put(sizez, result);
                    }
                    return result;
                }
                findingNonZeroStep = false;
            }
        }

        // binarySearchDebugPrint("found halting >= than 99% at step {}\n", .{currentStep});
    }
    // }
    return null;
}

pub fn binarySearchMemoryGivenStepsNPercentHalting(
    chosenSize: u32, //
    chosenSteps: u32,
    percentHalted: f64,
    //TODO add a cache for this??
    allocator: std.mem.Allocator,
    percentIgnoreInput: f64,
    randomSeed: ?u32,
    hashmapInput: ?*exp6memCountHashmap,
    totalsMapInput: ?*exp6memCountHashmap,
    maxInterpreterBytes: u32,
) !?u32 {
    const percentIgnore = if (chosenSize < minSizeForIgnore) 0.0 else percentIgnoreInput;

    var first = try BigIntManaged.init(allocator);
    var last = try BigIntManaged.init(allocator);
    var size = try BigIntManaged.init(allocator);

    var hashmapInternal = exp6memCountHashmap.init(allocator);
    var totalsMapInternal = exp6memCountHashmap.init(allocator);
    // var haltedStepHashmap = std.AutoHashMap(u32, u32).init(allocator);
    // var sizeTypedHashmap = exp6SizeTypeHashmap.init(allocator);

    defer {
        size.deinit();
        first.deinit();
        last.deinit();

        hashmapInternal.deinit();
        totalsMapInternal.deinit();
        // haltedStepHashmap.deinit();
        // sizeTypedHashmap.deinit();
    }

    // for (startSize..lastSize) |chosenSizeUsize| {
    // const chosenSize: u32 = @intCast(chosenSizeUsize);
    // const chosenSize: u32 = sizez;

    // binarySearchDebugPrint("\n", .{});
    try size.set(chosenSize);

    // this only succeeds if the size "exists" because not all numbers correspond to a size
    const indicesSuceed = try firstIndexSeenForProgramSize(&first, &size, allocator) and try lastIndexSeenForProgramSize(&last, &size, allocator);

    try last.addScalar(&last, 1);

    if (indicesSuceed) {
        // binarySearchDebugPrint("trying size {} -> first {d}, last {d}\n", .{ chosenSize, first, last });

        var currentMem: u32 = 1024;

        // dont think this is a good idea
        // if (percentIgnore == null and sizez >= minSizeForGuessing) {
        //     currentStep = try binarySearchNPercentHaltingStepForSize(
        //         sizez, //
        //         percentHalted,
        //         cacheMaybe,
        //         allocator,
        //         0.8,
        //         randomSeed orelse 42,
        //     ) orelse 20;
        // }

        var start: u32 = currentMem;
        var end: u32 = currentMem * 2;
        var middle: u32 = ((end - start) / 2) + start;
        var end99Halted = false;
        currentMem = end;

        var findingNonZeroStep = true;
        while (findingNonZeroStep) {
            if (currentMem > maxInterpreterBytes) {
                binarySearchDebugPrint("hit maximum bytes! {}\n", .{currentMem});
                return null;
            }

            const interprerBuffer = try allocator.alloc(u8, currentMem);
            defer allocator.free(interprerBuffer);
            var interpreterAllocator = std.heap.FixedBufferAllocator.init(interprerBuffer);
            binarySearchDebugPrint("N% mem halting: size {}, trying {} mem {} steps, S {}, M {}, E {}\n", .{ chosenSize, currentMem, chosenSteps, start, middle, end });

            // var prng: std.Random.DefaultPrng = .init(randomSeed orelse 42);

            // const randomArgs: exp6RandomArgs = .{
            //     .prng = if (percentIgnore != null) &prng else null,
            //     .percentIgnore = percentIgnore,
            //     .sizeStartIgnore = minSizeForSampling, //for safety
            // };

            const modeArgs: exp6modeArgs = .{
                .stepsAndMemory = .{
                    .args = .{
                        .countHashmap = if (hashmapInput) |the| the else &hashmapInternal,
                        .totalsHashmap = if (totalsMapInput) |the| the else &totalsMapInternal,
                    },
                },
            };

            const args: exp6args = .{
                .allocator = allocator, //
                .interpreterAllocator = &interpreterAllocator,
                .chosenSize = chosenSize,
                .chosenSteps = chosenSteps,
                .chosenMemory = currentMem,

                // .haltedStepHashmap = &haltedStepHashmap,

                .modeArgs = modeArgs,
                .randomArgs = null,
            };
            // args.percentIgnore = 0.0;
            streamUniqueRandomBinaryStrsRange(allocator, first, last, 1.0 - percentIgnore, randomSeed orelse 42, &exp6, args, false) catch |err| {
                if (err == error.Else) {
                    binarySearchDebugPrint("error else from entire binary search?\n", .{});
                    return err;
                }
            };
            // streamBinaryStrsRange(first, last, &exp6, args, false) catch |err| {
            //     if (err == error.Else) {
            //         return err;
            //     }
            // };

            var halted99M: bool = false;

            const totalMapM = modeArgs.stepsAndMemory.args.totalsHashmap.get(chosenSize);
            const haltedMapM = modeArgs.stepsAndMemory.args.countHashmap.get(chosenSize);

            if (totalMapM != null and haltedMapM != null) {
                const stepAndMemKey: StepAndMem = .{ .step = chosenSteps, .mem = currentMem };
                const haltedM = haltedMapM.?.get(stepAndMemKey);
                const totalM = totalMapM.?.get(stepAndMemKey);
                if (haltedM != null and totalM != null) {
                    const halted: f64 = haltedM.?;
                    const total: f64 = totalM.?;
                    const freqHalted = halted / total;

                    binarySearchDebugPrint("{} halted, {} total, {}% halted\n", .{ halted, total, freqHalted * 100 });

                    const greater99 = freqHalted >= percentHalted;
                    if (greater99) halted99M = true;
                }
            } else { //count is probably empty meaning 0% halted
                if (totalMapM != null and haltedMapM == null) {
                    binarySearchDebugPrint("{} total, 0% halted\n", .{totalMapM.?.count()});
                } else {
                    binarySearchDebugPrint("ttoals: {?}, count:{?}\n", .{ totalMapM, haltedMapM });
                }
            }

            if (end99Halted == false) {
                if (halted99M) {
                    end99Halted = true;
                    // end = middle;
                    middle = ((end - start) / 2) + start;
                    currentMem = middle;
                } else {
                    start = end;
                    end = end * 2;
                    middle = ((end - start) / 2) + start;
                    currentMem = end;
                }
            } else {
                if (halted99M) {
                    end = middle;
                } else {
                    start = middle;
                }
                middle = ((end - start) / 2) + start;
                currentMem = middle;
            }

            if (end == start + 1) {
                if (halted99M) {
                    // const result:StepAndMem = .{.step = currentStep, .mem = interpBytesAmount};
                    const result = currentMem;
                    // binarySearchDebugPrint("{},{}\n", .{ chosenSize, result });
                    // if (cacheMaybe) |cache| {
                    //     try cache.put(chosenSize, result);
                    // }
                    return result;
                } else {
                    const result = currentMem + 1;
                    // binarySearchDebugPrint("{},{}\n", .{ chosenSize, result });
                    // if (cacheMaybe) |cache| {
                    //     try cache.put(chosenSize, result);
                    // }
                    return result;
                }
                findingNonZeroStep = false;
            }
        }

        // binarySearchDebugPrint("found halting >= than 99% at step {}\n", .{currentStep});
    }
    return null;
}

pub fn estimateStepAndMemoryForSizeNPercentHalting(chosenSize: u32, percentHalting: f64, percentIgnore: f64, randomSeed: u32, maxInterpreterMemoryBytes: u32, allocator: std.mem.Allocator) !?StepAndMem {
    // const chosenSize: u32 = @intCast(chosenSizeUsize);

    // const haltingStep = try T3.sampleEstimateHaltingNStep(chosenSize, percentHaltedEnding, percentIgnore, randomSeed, arena, maxInterpreterMemoryBytes, init.io);
    var haltingStep: ?u32 = try T3.sampledBinarySearchNPercentHaltingStepForSize(chosenSize, percentHalting, null, allocator, maxInterpreterMemoryBytes, percentIgnore, randomSeed, null, null);

    if (haltingStep == null) return null;

    haltingStep.? = @intFromFloat(@as(f64, @floatFromInt(haltingStep.? + 2)) * 1.3); // increase 30% i suppose

    // std.debug.print("fonud halting step: {?}\n", .{haltingStep});

    var memVal = try T3.binarySearchMemoryGivenStepsNPercentHalting(chosenSize, //
        haltingStep.?, percentHalting, allocator, percentIgnore, randomSeed, null, null, maxInterpreterMemoryBytes);

    //same treatment
    memVal.? = @intFromFloat(@as(f64, @floatFromInt(memVal.? + 2)) * 1.3); // increase 30% i suppose

    if (haltingStep != null and memVal != null) {
        // std.debug.print("{},{},{}\n", .{ chosenSizeUsize, haltingStep.?, memVal.? });
        const returnStuff: StepAndMem = .{ .mem = memVal.?, .step = haltingStep.? };
        return returnStuff;
    }
    return null;
}

pub const exp5HashmapKey = struct {
    size: u32, //
    steps: u32,
};
// const exp5SizeAndType = struct { size: u32, sType: T3.SizeTypeDivTen };
//map<size -> map<steps -> haltedCount >>
pub const exp5stepHaltedHashmapType = std.AutoHashMap(u32, u32);
pub const exp5hashmapType = std.AutoHashMap(u32, exp5stepHaltedHashmapType);

pub const exp5freqHashmapType = std.AutoHashMap(u32, f64);
pub const exp5freqHashmapTypeType = std.AutoHashMap(u32, *exp5freqHashmapType);

const exp5FitnessContext = struct {
    hashmap: *exp5freqHashmapTypeType,
    chosenSize: u32,
    // startStep: u32,
};
var exp5FitnessGlobalContext: exp5FitnessContext = undefined;

// y = 1/(1 + e^(-a * (x - b)))
// a is between 0 and 1
// b is just greater than 0
fn exp5model(x: f64, a: f64, b: f64) f64 {
    // const eF: f64 = std.math.e;
    // return 1.0 / (1.0 + std.math.pow(f64, eF, (-a * (x - b))));

    const eBF = BigFloat.init(std.math.e);
    const oneBF = BigFloat.init(1);
    const xBF = BigFloat.init(x);
    const aBF = BigFloat.init(a);
    const bBF = BigFloat.init(b);

    const result = oneBF.div(oneBF.add(eBF.pow(aBF.neg().mul(xBF.sub(bBF)))));

    return result.toFloat(f64);
}

//RMSE MAE
fn exp5fitness(genome: []BigFloat) f64 {
    var sum: f64 = 0.0;
    var n: f64 = 0;

    var minDiff = [2]f64{ 99999999999, 0.0 };
    var maxDiff = [2]f64{ -1.0, 0.0 };

    var iter = exp5FitnessGlobalContext.hashmap.get(exp5FitnessGlobalContext.chosenSize).?.iterator();

    const a = genome[0].toFloat(f64);
    const b = genome[1].toFloat(f64);
    while (iter.next()) |entry| {
        const x: f64 = entry.key_ptr.*;
        const actual_y: f64 = entry.value_ptr.*;

        // if (!std.math.isFinite(a) or !std.math.isFinite(b)) {
        //     std.debug.print("a or b is not finite!?\n", .{});
        // }

        const predicted_y: f64 = exp5model(x, a, b);
        const diff: f64 = actual_y - predicted_y;

        if (x < minDiff[0]) minDiff = [2]f64{ x, @abs(diff) };
        if (x > maxDiff[0]) maxDiff = [2]f64{ x, @abs(diff) };

        // if (!std.math.isFinite(predicted_y) or !std.math.isFinite(diff)) {
        //     std.debug.print("predicted_y or diff is not finite!?\n", .{});
        // }
        sum += diff * diff; //RMSE
        // sum += @abs(diff); //MAE
        n += 1;
    }
    // if (!std.math.isFinite(sum)) {
    //     std.debug.print("sum is not finite!?\n", .{});
    // }

    // penalize getting the last and first points off by a bit
    var penaltySum: f64 = (maxDiff[1] + minDiff[1]) * n * 0.5;
    // if (exp5FitnessGlobalContext.chosenSize == 43) //
    //     std.debug.print("min {},{} max {},{} penalty {}\n", .{ minDiff[0], minDiff[1], maxDiff[0], maxDiff[1], penaltySum });

    // no cheating
    if (a > 1.0 or b < 0) penaltySum += 100;

    const result = -@sqrt((sum + penaltySum) / n); //RMSE
    // const result = -sum / n; //MAE
    // if (!std.math.isFinite(result)) {
    //     std.debug.print("fitness is not finite!?\n", .{});
    // }
    return result;
    // return -sum;
}

//TODO merge this with exp5 (with steps) using same technique as i did with exp6
pub const exp5WithMemoryXY = struct { x: u32, y: u32 };
pub const exp5WithMemoryfreqHashmapType = std.AutoHashMap(exp5WithMemoryXY, f64);
pub const exp5WithMemoryfreqHashmapTypeType = std.AutoHashMap(u32, *exp5WithMemoryfreqHashmapType);

const exp5WithMemoryFitnessContext = struct {
    hashmap: *exp5WithMemoryfreqHashmapTypeType,
    chosenSize: u32,
    // startStep: u32,
};
var exp5WithMemoryFitnessGlobalContext: exp5WithMemoryFitnessContext = undefined;
// z =(1/(1 + e^(-a * (x - b)))) * (1/(1 + e^(-c * (y - d))))
// a,c is between 0 and 1
// b,d is just greater than 0
fn exp5WithMemoryModel(x: f64, y: f64, a: f64, b: f64, c: f64, d: f64) f64 {
    const eBF = BigFloat.init(std.math.e);
    const oneBF = BigFloat.init(1);
    const xBF = BigFloat.init(x);
    const yBF = BigFloat.init(y);
    const aBF = BigFloat.init(a);
    const bBF = BigFloat.init(b);
    const cBF = BigFloat.init(c);
    const dBF = BigFloat.init(d);

    const result1 = oneBF.div(oneBF.add(eBF.pow(aBF.neg().mul(xBF.sub(bBF)))));
    const result2 = oneBF.div(oneBF.add(eBF.pow(cBF.neg().mul(yBF.sub(dBF)))));

    return result1.mul(result2).toFloat(f64);
}

//RMSE MAE
fn exp5WithMemoryFitness(genome: []BigFloat) f64 {
    var sum: f64 = 0.0;
    var n: f64 = 0;

    // var minDiff = [2]f64{ 99999999999, 0.0 };
    // var maxDiff = [2]f64{ -1.0, 0.0 };

    var iter = exp5WithMemoryFitnessGlobalContext.hashmap.get(exp5WithMemoryFitnessGlobalContext.chosenSize).?.iterator();

    const a = genome[0].toFloat(f64);
    const b = genome[1].toFloat(f64);
    const c = genome[2].toFloat(f64);
    const d = genome[3].toFloat(f64);
    while (iter.next()) |entry| {
        const x: f64 = entry.key_ptr.*.x;
        const y: f64 = entry.key_ptr.*.y;
        const actual: f64 = entry.value_ptr.*;

        // if (!std.math.isFinite(a) or !std.math.isFinite(b)) {
        //     std.debug.print("a or b is not finite!?\n", .{});
        // }

        const predicted: f64 = exp5WithMemoryModel(x, y, a, b, c, d);
        const diff: f64 = actual - predicted;

        // if (x < minDiff[0]) minDiff = [2]f64{ x, @abs(diff) };
        // if (x > maxDiff[0]) maxDiff = [2]f64{ x, @abs(diff) };

        // if (!std.math.isFinite(predicted_y) or !std.math.isFinite(diff)) {
        //     std.debug.print("predicted_y or diff is not finite!?\n", .{});
        // }
        sum += diff * diff; //RMSE
        // sum += @abs(diff); //MAE
        n += 1;
    }
    // if (!std.math.isFinite(sum)) {
    //     std.debug.print("sum is not finite!?\n", .{});
    // }

    // penalize getting the last and first points off by a bit
    // var penaltySum: f64 = (maxDiff[1] + minDiff[1]) * n * 0.5;
    var penaltySum: f64 = 0.0;
    // if (exp5FitnessGlobalContext.chosenSize == 43) //
    //     std.debug.print("min {},{} max {},{} penalty {}\n", .{ minDiff[0], minDiff[1], maxDiff[0], maxDiff[1], penaltySum });

    // no cheating
    if (a > 1.0 or b < 0 or c > 1.0 or d < 0) penaltySum += 100;

    const result = -@sqrt((sum + penaltySum) / n); //RMSE
    // const result = -sum / n; //MAE
    // if (!std.math.isFinite(result)) {
    //     std.debug.print("fitness is not finite!?\n", .{});
    // }
    return result;
    // return -sum;
}

//TODO add an error which indicates when we stop reaching 0.99% which probably means too many OutOfMemory errors?
pub fn sampleEstimateHaltingNStep(
    chosenFreqSize: u32, //
    percentHaltedEnding: f64,
    percentIgnore: f64,
    randomSeed: comptime_int,
    allocator: std.mem.Allocator,
    interpreterMemoryBytes: u32,
    // interpreterAllocator: *std.heap.FixedBufferAllocator,
    ioM: ?std.Io,
    sizeFileM: ?std.Io.File,
) !?u32 {
    if (T3.sizeGetTypeDivTen(chosenFreqSize) == null) return null;

    // const percentIgnore = 0.0;

    var hashmap = exp6StepsHashmapType.init(allocator);
    defer hashmap.deinit();

    var totalsHashmap = exp6StepsHashmapType.init(allocator);
    defer totalsHashmap.deinit();

    var sizeFirstCache = T3.binarySearchFirstHaltingStepForSizeCache.init(allocator);
    defer sizeFirstCache.deinit();

    var a = try BigIntManaged.init(allocator);
    var b = try BigIntManaged.init(allocator);
    defer b.deinit();
    defer a.deinit();

    // const chosenFreqSize = 53;

    // const dataPoints = 20;
    // const sizeStartIgnore = null;

    const startStep = try T3.binarySearchFirstHaltingStepForSize(chosenFreqSize, &sizeFirstCache, allocator, interpreterMemoryBytes, percentIgnore, randomSeed, &hashmap, &totalsHashmap) orelse return null;
    const lastStep = (try T3.sampledBinarySearchNPercentHaltingStepForSize(chosenFreqSize, percentHaltedEnding, null, allocator, interpreterMemoryBytes, percentIgnore, randomSeed, &hashmap, &totalsHashmap) orelse return null) + 1;

    var bigSize = try BigIntManaged.init(allocator);
    defer bigSize.deinit();

    // if (chosenFreqSize != null) {
    try bigSize.set(chosenFreqSize);

    _ = try T3.firstIndexSeenForProgramSize(&a, &bigSize, allocator);
    _ = try T3.lastIndexSeenForProgramSize(&b, &bigSize, allocator);
    // }

    // var prng: std.Random.DefaultPrng = .init(42);
    // const args: exp5args = .{
    //     .allocator = allocator, //
    //     .hashmap = &hashmap,
    //     .totalsHashmap = &totalsHashmap,
    //     // .maxSteps = maxSteps,
    //     .startStep = startStep,
    //     .lastStep = lastStep,
    //     .dataPoints = dataPoints,
    //     .chosenSize = chosenFreqSize,
    //     .sizeFirstStepCache = &sizeFirstCache,
    //     .prng = &prng,
    //     .percentIgnore = percentIgnore,
    //     .sizeStartIgnore = sizeStartIgnore,
    // };
    // try T3.streamBinaryStrsRange(a, b, &exp5, args, false);

    // size -> (steps -> freq)
    var frequencyHashmap = exp5freqHashmapTypeType.init(allocator);
    defer frequencyHashmap.deinit();

    var it = totalsHashmap.iterator();
    // std.debug.print("\n", .{});
    //populate frequency hashmap
    while (it.next()) |entry| {
        const size = entry.key_ptr.*;

        // const totalCount = entry.value_ptr.*;

        const stepHaltedHashmapMaybe = hashmap.get(size);
        const stepTotalHashmapMaybe = totalsHashmap.get(size);

        // std.debug.print("\n\n size {} distribution:\n", .{size});
        if (stepHaltedHashmapMaybe != null and stepTotalHashmapMaybe != null) {
            const stepHaltedHashmap = stepHaltedHashmapMaybe.?;
            const stepTotalHashmap = stepTotalHashmapMaybe.?;

            // var haltedCounter: u32 = 0;
            // const totalTotalCount = totalCount * maxSteps;

            // const startStep = 1;
            // std.debug.print("populating frequencyHashmap from {} to {}\n", .{ startStep, lastStep + 1 });
            for (startStep..lastStep + 1) |stepsUsize| {
                const steps: u32 = @intCast(stepsUsize);

                const haltedCountMaybe = stepHaltedHashmap.get(steps);
                const totalMaybe = stepTotalHashmap.get(steps);

                var haltedFloat: f64 = 0;
                var totalFloat: f64 = 1;

                if (haltedCountMaybe != null and totalMaybe != null) {
                    // haltedCounter += haltedCount;
                    const haltedCount = haltedCountMaybe.?;
                    const total = totalMaybe.?;

                    haltedFloat = haltedCount;
                    totalFloat = total;

                    // if (size == chosenFreqSize) //
                    //     std.debug.print("{} halted out of {} in {} steps\n", .{ haltedCount, total, steps });

                    const percentHalted: f64 = haltedFloat / totalFloat;

                    var frequencyHashmap2Maybe = frequencyHashmap.get(size);
                    if (frequencyHashmap2Maybe) |_| {
                        try frequencyHashmap2Maybe.?.put(steps, percentHalted);
                    } else {
                        const newHashmap = try allocator.create(exp5freqHashmapType);
                        newHashmap.* = exp5freqHashmapType.init(allocator);

                        try frequencyHashmap.put(size, newHashmap);

                        try frequencyHashmap.get(size).?.put(steps, percentHalted);
                    }
                    // if (size == chosenFreqSize) //
                }
            }
        }
    }

    //save to file
    var hundredPercentHaltingStep: ?u32 = null;
    if (sizeFileM != null and ioM != null) {
        var sizeFile = sizeFileM.?;
        const io = ioM.?;
        // const sizeFile = try std.Io.Dir.cwd().createFile(io, "sizeHaltingFrequencies.txt", .{});
        defer sizeFile.close(io);

        var sizeFileBuf: [4000]u8 = undefined;
        var sizeFileWriter = sizeFile.writer(io, &sizeFileBuf);
        const sizeFileWriterPtr = &sizeFileWriter.interface;
        defer sizeFileWriter.flush() catch unreachable;

        // var lastPercentHalted: f64 = 0.0;
        var it2 = frequencyHashmap.iterator();
        while (it2.next()) |entry| {
            const size = entry.key_ptr.*;
            try sizeFileWriterPtr.print("\n\nsize {}:\n", .{size});
            const freqMap = entry.value_ptr.*;
            // const startStep = try T3.binarySearchFirstHaltingStepForSize(size, &sizeFirstCache, allocator, percentIgnore, null) orelse 1;
            for (startStep..lastStep + 1) |steps| {
                const percentHaltedMaybe: ?f64 = freqMap.get(@intCast(steps));

                if (percentHaltedMaybe) |percentHalted| {
                    try sizeFileWriterPtr.print("{},{}\n", .{ steps, percentHalted });
                    if (percentHalted == 1.0) {
                        hundredPercentHaltingStep = @intCast(steps);
                    }
                    // lastPercentHalted = percentHalted;
                } else {
                    // try sizeFileWriterPtr.print("{},{}\n", .{ steps, lastPercentHalted });
                }
            }
        }
    }

    // fit model to data
    debugPrint("size: {}\n", .{chosenFreqSize});
    {
        const genomeSize = 2;
        const mutateChance = 0.05;
        const crossoverChance = 0.33;
        const iterations = 1000;
        const populationSize = 100;
        const selectionTopPercent = 0.2;
        // const randomSeed = 42;

        exp5FitnessGlobalContext = .{ .chosenSize = chosenFreqSize, .hashmap = &frequencyHashmap };
        const fitnessFunc = exp5fitness;

        var geneticThing = BFGenetic.BFVectorEvolutionContext(genomeSize, fitnessFunc, mutateChance, crossoverChance, iterations, populationSize, selectionTopPercent, randomSeed){};

        geneticThing.epoch();

        const topIndividual = geneticThing.getPopulation()[0];

        debugPrint("\ttop fitness individual: a {f}, b {f}, fitness {}\n", .{ //
            topIndividual.genome[0],
            topIndividual.genome[1],
            topIndividual.fitness,
        });

        //calculate steps to get desired percent halting machines
        // const desiredHaltingPercent = 0.9999;

        const desiredProbBF = BigFloat.init(percentHaltedEnding);
        const varA = topIndividual.genome[0];
        const varB = topIndividual.genome[1];
        const oneBF = BigFloat.init(1);
        const eF = BigFloat.init(std.math.e);

        //courtesy of wolframalpha
        const endingSteps = (varA.mul(varB).sub(BigFloat.log2(oneBF.div(desiredProbBF).sub(oneBF)).div(BigFloat.log2(eF)))).div(varA);

        binarySearchDebugPrint("lastStep: {}, estimatedLastStep: {d}\n", .{ lastStep, endingSteps });

        const endingStepsInt: u32 = if ( //
            (chosenFreqSize < minSizeForIgnore and hundredPercentHaltingStep != null) //
            // or (percentIgnore > 0.6) //
            ) //
                @intFromFloat(@as(f64, @floatFromInt(hundredPercentHaltingStep.? + 1)) * 1.3) // increase 30% i suppose
            else if (percentIgnore > -1.0) //
                @intFromFloat(@as(f64, @floatFromInt(lastStep + 1)) * 1.3) // increase 30% i suppose
            else
                @intFromFloat(endingSteps.toFloat(f64) + 2);
        // const endingStepsInt: u32 = std.math.lossyCast(u32, endingSteps.toFloat(f64));

        debugPrint("\tsteps to achieve {}% halting is ~{}\n", .{ percentHaltedEnding * 100, endingStepsInt });

        return endingStepsInt;
    }

    return null;
    // std.debug.print("\n", .{});
}

// binary str -> jot parse -> reduce -> binary str
pub fn caca(binarySlice: []const u8, maxTraverses: ?u32, allocator: std.mem.Allocator) !SizeAndOutput {
    var interpreter = LambdaInterpreter.init(allocator);
    var jotParser: JotParser = JotParserMake(allocator);
    defer jotParser.deinit();

    debugPrint("\t\"{s}\"\n\t=jotParse> ", .{binarySlice});
    const jotExpr = try jotParser.parse(binarySlice);
    if (debugPrintON) jotExpr.traversePrint();

    if (interpreter.reduceUntilCantReduce(jotExpr, maxTraverses, null, true)) |reducedExpr| {
        debugPrint("\n\t=reduce> ", .{});
        if (debugPrintON) reducedExpr.traversePrint();
        // const blcSliceString2 = try loadBlcExpressionToSlice(reducedExpr, allocator);
        // const blcSliceString2 = try mixedRadixLoadExprToSlice(reducedExpr, allocator);
        const blcSliceString2 = try ArithmeticCoder.encode(reducedExpr, allocator);

        debugPrint("\n\t=blc> ", .{});
        debugPrint("\"{s}\"", .{blcSliceString2});
        return .{ .inputSize = jotExpr.size(), .output = blcSliceString2 };
    } else |err| switch (err) {
        else => {
            debugPrint("\n\t=reduce> {}", .{err});
            return err;
        },
    }
}

// binary str -> jot parse -> reduce -> binary str
pub fn caca2(binarySlice: []const u8, maxTraverses: ?u32, allocator: std.mem.Allocator) !?SizeAndOutput {
    var interpreter = LambdaInterpreter.init(allocator);
    var jotParser: JotParser = JotParserMake(allocator);
    defer jotParser.deinit();

    debugPrint("\t\"{s}\"\n\t=jotParse> ", .{binarySlice});
    const jotExpr = try jotParser.parse(binarySlice);
    if (debugPrintON) jotExpr.traversePrint();

    if (interpreter.reduceUntilCantReduce(jotExpr, maxTraverses, null, true)) |reducedExpr| {
        debugPrint("\n\t=reduce> ", .{});
        if (debugPrintON) reducedExpr.traversePrint();
        // const blcSliceString2 = try loadBlcExpressionToSlice(reducedExpr, allocator);
        // const blcSliceString2 = try mixedRadixLoadExprToSlice(reducedExpr, allocator);
        const blcSliceString2 = try ArithmeticCoder.encode(reducedExpr, allocator);
        debugPrint("\n\t=blc> ", .{});
        debugPrint("\"{s}\"", .{blcSliceString2});
        return .{ .inputSize = jotExpr.size(), .output = blcSliceString2 };
    } else |err| switch (err) {
        error.OutOfMemory => {
            return null;
        },
        error.MAX_TRAVERSES => {
            return null;
        },
        else => {
            debugPrint("\n\t=reduce> {}", .{err});
            return err;
        },
    }
}

pub fn cacaHalted(binarySlice: []const u8, maxTraverses: ?u32, allocator: std.mem.Allocator) !bool {
    var interpreter = LambdaInterpreter.init(allocator);
    var jotParser: JotParser = JotParserMake(allocator);
    defer jotParser.deinit();

    debugPrint("\t\"{s}\"\n\t=jotParse> ", .{binarySlice});
    const jotExpr = try jotParser.parse(binarySlice);
    if (debugPrintON) jotExpr.traversePrint();

    if (interpreter.reduceUntilCantReduce(jotExpr, maxTraverses, null, true)) |reducedExpr| {
        debugPrint("\n\t=reduce> ", .{});
        if (debugPrintON) reducedExpr.traversePrint();
        // const blcSliceString2 = try loadBlcExpressionToSlice(reducedExpr, allocator);
        // const blcSliceString2 = try mixedRadixLoadExprToSlice(reducedExpr, allocator);
        const blcSliceString2 = try ArithmeticCoder.encode(reducedExpr, allocator);

        debugPrint("\n\t=blc> ", .{});
        debugPrint("\"{s}\"", .{blcSliceString2});
        // return .{ .inputSize = jotExpr.size(), .output = blcSliceString2 };
        return true;
    } else |err| switch (err) {
        else => {
            debugPrint("\n\t=reduce> {}", .{err});
            if (err == error.MAX_TRAVERSES) return false else return err;
        },
    }
}
