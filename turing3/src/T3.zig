const std = @import("std");

const LambdaParser = @import("LambdaParser.zig").LambdaParser;
const LambdaInterpreter = @import("LambdaInterpreter.zig").LambdaInterpreter;

const LambdaBuilder = @import("LambdaBuilder.zig").LambdaBuilder;

const JotParser = @import("JotParser.zig").JotParser;
const JotParserMake = @import("JotParser.zig").JotParserMake;
const blcPrintExpression = @import("Blc.zig").blcPrintExpression;
const loadBlcExpressionToSlice = @import("Blc.zig").loadBlcExpressionToSlice;

const BigFloat = @import("BFP").BigFloat(.{ .Significand = f64, .Exponent = i64 });
const BigIntManaged = std.math.big.int.Managed;

const debugPrintON = false;
fn debugPrint(comptime fmt: []const u8, args: anytype) void {
    if (debugPrintON) std.debug.print(fmt, args);
}

pub const SizeAndOutput = struct { inputSize: u32, output: []const u8 };

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
pub fn maxEstimatedProgramsOfSize(size: usize) !BigFloat {
    const oneThing = BigFloat.init(1.01114);
    const nineThing = BigFloat.init(9.28113);
    const sizeThing = BigFloat.init(size);
    const pointTwo = BigFloat.init(0.241582);

    return oneThing.pow(nineThing.mul(sizeThing)).mul(pointTwo);
}

pub fn getSize(binarySlice: []const u8, allocator: std.mem.Allocator) !u32 {
    var jotParser: JotParser = JotParserMake(allocator);
    defer jotParser.deinit();

    debugPrint("\t\"{s}\"\n\t=jotParse> ", .{binarySlice});
    const jotExpr = try jotParser.parse(binarySlice);
    if (debugPrintON) jotExpr.traversePrint();
    return jotExpr.size();
}

// binary str -> jot parse -> reduce -> binary str
pub fn caca(binarySlice: []const u8, maxTraverses: ?u32, allocator: std.mem.Allocator) !SizeAndOutput {
    var interpreter = LambdaInterpreter.init(allocator);
    var jotParser: JotParser = JotParserMake(allocator);

    debugPrint("\t\"{s}\"\n\t=jotParse> ", .{binarySlice});
    const jotExpr = try jotParser.parse(binarySlice);
    if (debugPrintON) jotExpr.traversePrint();

    if (interpreter.reduceUntilCantReduce(jotExpr, maxTraverses, null, true)) |reducedExpr| {
        debugPrint("\n\t=reduce> ", .{});
        if (debugPrintON) reducedExpr.traversePrint();
        const blcSliceString2 = try loadBlcExpressionToSlice(reducedExpr, allocator);
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
