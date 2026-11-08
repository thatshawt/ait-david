const std = @import("std");
const Io = std.Io;

const now = std.Io.Clock.now;

const turing3 = @import("turing3");

const LambdaParser = @import("LambdaParser.zig").LambdaParser;
const LambdaInterpreter = @import("LambdaInterpreter.zig").LambdaInterpreter;

const LambdaBuilder = @import("LambdaBuilder.zig").LambdaBuilder;

const JotParser = @import("JotParser.zig").JotParser;
const JotParserMake = @import("JotParser.zig").JotParserMake;

const BigIntManaged = std.math.big.int.Managed;
const cantorPair = @import("PairingFunction.zig").cantorPair;
const cantorUnpair = @import("PairingFunction.zig").cantorUnpair;

const generate = @import("LambdaEnumerate.zig").generate;

const blcPrintExpression = @import("Blc.zig").blcPrintExpression;
const loadBlcExpressionToSlice = @import("Blc.zig").loadBlcExpressionToSlice;

const T3 = @import("T3.zig");

const BigFloat = @import("BFP").BigFloat(.{ .Significand = f64, .Exponent = i64 });

const BFGenetic = @import("BFVectorGeneticEvolution.zig");

const ArrayList = std.ArrayList;
// https://lambster.dev/ very helpful for confirming what expressions should reduce to

fn replaceRetSize(comptime T: type, input: []const T, needle: []const T, replacement: []const T, output: []T) usize {
    const size = std.mem.replacementSize(T, input, needle, replacement);
    _ = std.mem.replace(T, input, needle, replacement, output[0..size]);
    return size;
}

fn doStuff(allocator: std.mem.Allocator) !void {
    const stuffity_stuff = 1 + 1;
    std.debug.print("did someting!? {} \n", .{stuffity_stuff});

    //   ( /(( /((1 (2 2))) /((1 (2 2))) )) 99 )

    const Lzero = "/( /( 1))";
    const Lincr = "/(/(/( (2 ((3 2) 1)) )))";
    const Lplus = "/(/(((2 /(/(/( (2 ((3 2) 1)) )))) 1)))";
    const Ltimes = "/( /( ((2 (/(/(((2 /(/(/( (2 ((3 2) 1)) )))) 1))) 1)) /(/( 1)))))";

    // const stuffityLambda = "(incr zero)";
    // const stuffityLambda = "(incr (incr zero))";
    // const stuffityLambda = "((times (incr (incr zero))) (incr (incr zero)))";
    // const stuffityLambda = "(/(1) (2 1))";
    // const stuffityLambda = "/(/(((/(/(2)) 1) (2 1))))";
    // const stuffityLambda = "/( (/(/(/(3))) 1) )";
    // const stuffityLambda = "((/(1) /(/(/(((3 1) (2 1)))))) /(/(2)))";
    const stuffityLambda = "(/(/( ((4 2) /((1 3))) )) /((5 1)))"; // => /(((3 /((6 1))) /((1 /((7 1))))))
    // const stuffityLambda = "(/(/(2)) 1)"; // => /(2)

    const maxProgramLength = 500;
    var output1: [maxProgramLength]u8 = undefined;
    var output2: [maxProgramLength]u8 = undefined;
    @memcpy(output1[0..stuffityLambda.len], stuffityLambda[0..]);

    var size = stuffityLambda.len;
    size = replaceRetSize(u8, output1[0..size], "incr", Lincr, output2[0..]);
    size = replaceRetSize(u8, output2[0..size], "plus", Lplus, output1[0..]);
    size = replaceRetSize(u8, output1[0..size], "zero", Lzero, output2[0..]);
    size = replaceRetSize(u8, output2[0..size], "times", Ltimes, output1[0..]);

    const lambdaText = output1[0..size];
    // const lambdaText = " (/(( /((1 /((1(1 3))))) /((1 /((1(1 3))))) )) 99 ) "; //Z fixed-point
    // const lambdaText = "( /( ( 2(2 1))) 99 )"; //idk
    // const lambdaText = "/( /( /((2 ((3 2) 1)))))"; //a test
    // const lambdaText = "( /((1 1)) /((1 1)) )"; //omega non-terminating reduction
    std.debug.print("reducing: {s}\n", .{lambdaText});

    const topExpr = try LambdaParser.parse(allocator, lambdaText);
    // topExpr.traversePrint();
    // std.debug.print("\n", .{});

    var interpreter = LambdaInterpreter.init(allocator);

    if (interpreter.reduceUntilCantReduce(topExpr, 400, null, true)) |reducedExpr| {
        std.debug.print("result: ", .{});
        reducedExpr.traversePrint();
        std.debug.print("\n\n", .{});
    } else |err| switch (err) {
        else => {
            std.debug.print("\n{}\n", .{err});
        },
    }

    // cantor pair
    var cantorResult = try BigIntManaged.init(allocator);
    defer cantorResult.deinit();

    var a = try BigIntManaged.init(allocator);
    var b = try BigIntManaged.init(allocator);
    defer a.deinit();
    defer b.deinit();

    // var formatBuff: [100]u8 = undefined;

    // var stdout_writer = std.Io.File.writer(std.Io.File.stdout(), std.Io, stdoutBuff);

    const aMax = 5;
    const bMax = 5;
    for (0..aMax) |aInt| {
        for (0..bMax) |bInt| {
            try a.set(aInt);
            try b.set(bInt);
            try cantorPair(allocator, &cantorResult, &a, &b);

            const resultStr = try cantorResult.toString(allocator, 10, std.fmt.Case.lower);
            defer allocator.free(resultStr);

            std.debug.print("({},{}) => {s}\n", .{ aInt, bInt, resultStr });

            try a.set(99999999);
            try b.set(99999999);
            try cantorUnpair(allocator, &cantorResult, &a, &b);

            const aString = try a.toString(allocator, 10, std.fmt.Case.lower);
            const bString = try b.toString(allocator, 10, std.fmt.Case.lower);
            defer allocator.free(aString);
            defer allocator.free(bString);

            std.debug.print("({s},{s}) <= {s}\n\n", .{ aString, bString, resultStr });
        }
    }

    var i = try BigIntManaged.init(allocator);
    defer i.deinit();

    const iMax = 10;
    var iInt: u32 = 0;
    var list: ArrayList(u8) = .empty;
    // list.init(allocator);
    defer list.deinit(allocator);

    var jotParser: JotParser = JotParserMake(allocator);
    var fmtBuffer: [400]u8 = undefined;

    while (iInt < iMax) : (iInt += 1) {
        // if(iInt % 2 == 0)continue;
        @memset(&fmtBuffer, 0);

        try i.set(iInt);

        const binarySlice = try std.fmt.bufPrint(&fmtBuffer, "{b}", .{iInt});
        // std.debug.print("{s}|{s}\n\n", .{fmtBuffer});

        std.debug.print("{}\n\t=> \"{s}\"\n\t\t=generateExpression> ", .{ iInt, fmtBuffer });
        const thing = try generate(allocator, &i);
        // std.debug.print("   ",.{});
        thing.traversePrint();

        const blcSliceString = try loadBlcExpressionToSlice(thing, allocator);
        std.debug.print("\n\t\t=blc> ", .{});
        std.debug.print("\"{s}\"", .{blcSliceString});

        try i.setString(2, blcSliceString);
        const blcNumber = try i.toString(allocator, 10, std.fmt.Case.lower);
        defer allocator.free(blcNumber);
        std.debug.print("\n\t\t=> {s}", .{blcNumber});

        std.debug.print("\n\n\t\t=jotParse> ", .{});
        const jotExpr = try jotParser.parse(binarySlice);
        jotExpr.traversePrint();

        std.debug.print("\n\t\t\t=size> {}", .{jotExpr.size()});

        if (interpreter.reduceUntilCantReduce(jotExpr, 100, null, true)) |reducedExpr| {
            std.debug.print("\n\t\t=reduce> ", .{});
            reducedExpr.traversePrint();
            const blcSliceString2 = try loadBlcExpressionToSlice(reducedExpr, allocator);
            std.debug.print("\n\t\t=blc> ", .{});
            std.debug.print("\"{s}\"", .{blcSliceString2});
        } else |err| switch (err) {
            else => {
                std.debug.print("\n\t\t=reduce> {}", .{err});
            },
        }

        std.debug.print("\n\n", .{});
    }

    var builder: LambdaBuilder = .{ .arena = std.heap.ArenaAllocator.init(allocator) };
    (try builder.apply(.{ .expr = try builder.apply(.{ .text = "999999" }, .{ .text = "/(/(/(((3 1) (2 1)))))" }) }, .{ .text = "/(/(2))" })).traversePrint();
}

fn f128PowNasty(a: f128, b: f128) f128 {
    var c: f128 = 1;
    for (0..(@as(usize, std.math.ceil(b)))) |_| {
        c *= a;
    }
    return c;
}

// fn streamStepsRange(
//     // allocator: std.mem.Allocator, //
//     a: BigIntManaged,
//     b: BigIntManaged,
//     comptime foreach: *const fn (*BigIntManaged, []const u8, args: anytype) error{ Yield, Else }!void,
//     args: anytype,
// ) !void {
//     var fmtBuffer: [400]u8 = undefined;

//     var i: BigIntManaged = try a.clone();
//     defer i.deinit();

//     var progressI: BigIntManaged = try b.clone();
//     defer progressI.deinit();

//     var r: BigIntManaged = try b.clone();
//     defer r.deinit();

//     var ten: BigIntManaged = try b.clone();
//     defer ten.deinit();
//     try ten.set(10);

//     try progressI.divFloor(&r, &b, &ten);
//     try r.copy(progressI.toConst());

//     var onwards = true;
//     var percent: u32 = 10;
//     while (onwards) {
//         if (progressI.eql(i)) {
//             try progressI.add(&progressI, &r);
//             std.debug.print("{d}% reached\n", .{percent});
//             percent += 10;
//         }
//         @memset(&fmtBuffer, 0);

//         const binarySlice = try std.fmt.bufPrint(&fmtBuffer, "{b}", .{i});

//         // std.debug.print("\n{s}", .{binarySlice});

//         try foreach(&i, binarySlice, args);

//         onwards = switch (i.order(b)) {
//             std.math.Order.eq => false,
//             std.math.Order.gt => false,
//             std.math.Order.lt => true,
//         };
//         if (onwards) {
//             try i.addScalar(&i, 1);
//         }
//     }
// }

fn exp1_noteOutputs(_: *BigIntManaged, input: []const u8, args: anytype) error{ Yield, Else }!void {
    // const input = "111";
    // std.debug.print("\ncaca thing:\n", .{});
    const result = T3.caca(input, 1000, args) catch return error.Else;
    std.debug.print("\n{s} size {} =>, {s}", .{ input, result.inputSize, result.output });
}

// const args2key = struct { input: []const u8, size: u32 };
// const args2hashmap = std.AutoHashMap(args2key, bool);
const args2hashmap = std.StringHashMap(bool);
const exp2Args = struct {
    allocator: std.mem.Allocator, //
    arena: std.heap.ArenaAllocator,
    hashmap: *args2hashmap,
    traverses: u32,
};
fn exp2_noteHaltedInputs(_: *BigIntManaged, input: []const u8, args: anytype) error{ Yield, Else }!void {
    var theHashmap = @as(*args2hashmap, args.hashmap);

    var args2: exp2Args = args;

    var arenaAlloc = args2.arena.allocator();

    const hashmapInput = arenaAlloc.alloc(u8, input.len) catch return error.Else;
    @memcpy(hashmapInput, input);

    const size = T3.getSize(input, args.allocator) catch {
        return error.Else;
    };

    // const key: args2key = .{ .input = input, .size = size };

    const result = T3.caca(input, args2.traverses, args.allocator) catch |err| {
        if (err == error.MAX_TRAVERSES) {
            theHashmap.put(hashmapInput, false) catch return error.Else;
            std.debug.print("\n{s} size {} =>, {s}", .{ input, size, "MAX TRAVERSES" });
            // const returnThing: T3.SizeAndOutput = .{ .inputSize = size, .output = "" };
            return;
        } else {
            return error.Else;
        }
    };

    std.debug.print("\n{s} size {} =>, {s}", .{ input, result.inputSize, result.output });

    theHashmap.put(hashmapInput, true) catch return error.Else;
}

// size -> (firstSeenIndex, lastSeenIndex)
const exp3indexHashmap = std.AutoHashMap(u32, struct {
    firstSeenIndex: *BigIntManaged, //
    lastSeenIndex: *BigIntManaged,
});
const exp3args = struct {
    allocator: std.mem.Allocator, //
    hashmap: *std.AutoHashMap(u32, u32),
    indexHashmap: *exp3indexHashmap,
    sizeFileWriter: *std.Io.Writer,
    chosenSize: ?u32 = null,
};
fn experiment3(index: *BigIntManaged, input: []const u8, argz: anytype) error{ Yield, Else }!void {
    const args: exp3args = argz;

    var theHashmap = args.hashmap;

    const size = T3.getSize(input, args.allocator) catch {
        return error.Else;
    };
    if (args.chosenSize != null and args.chosenSize.? != size) return;

    // const indexInt = index.toInt(u32) catch unreachable;
    // if (indexInt < 100_000) {
    //     // std.debug.print("{d},{}\n", .{ indexInt, size });
    //     args.sizeFileWriter.print("{d},{}\n", .{ indexInt, size }) catch unreachable;
    //     args.sizeFileWriter.flush() catch unreachable;
    // }

    const thing = theHashmap.getOrPut(size) catch return error.Else;
    if (thing.found_existing) {
        thing.value_ptr.* += 1;
    } else {
        thing.value_ptr.* = 1;
    }

    const indexStructMaybe = args.indexHashmap.get(size);
    if (indexStructMaybe) |indexStruct| {
        indexStruct.lastSeenIndex.copy(index.toConst()) catch return error.Else;
    } else {
        const firstSeen = args.allocator.create(BigIntManaged) catch return error.Else;
        const lastSeen = args.allocator.create(BigIntManaged) catch return error.Else;

        firstSeen.* = BigIntManaged.init(args.allocator) catch return error.Else;
        lastSeen.* = BigIntManaged.init(args.allocator) catch return error.Else;

        firstSeen.copy(index.toConst()) catch return error.Else;
        lastSeen.copy(index.toConst()) catch return error.Else;
        args.indexHashmap.put(size, .{
            .firstSeenIndex = firstSeen, //
            .lastSeenIndex = lastSeen,
        }) catch return error.Else;
    }
}

const exp5HashmapKey = struct {
    size: u32, //
    steps: u32,
};

// const exp5SizeAndType = struct { size: u32, sType: T3.SizeTypeDivTen };
//map<size -> map<steps -> haltedCount >>
const exp5stepHaltedHashmapType = std.AutoHashMap(u32, u32);
const exp5hashmapType = std.AutoHashMap(u32, exp5stepHaltedHashmapType);

// const exp5sizeTypedHashmapType = std.AutoHashMap(exp5SizeAndType, exp5stepHaltedHashmapType);

// const exp5AndType = struct { size: u32, sType: T3.SizeTypeDivTen };
// const exp5sizeTypedTotalsHashmapType = std.AutoHashMap(exp5SizeAndType, exp5stepHaltedHashmapType);
const exp5args = struct {
    allocator: std.mem.Allocator, //
    hashmap: *exp5hashmapType,
    totalsHashmap: *exp5hashmapType, // size -> <steps -> total>
    // maxSteps: u32,
    startStep: u32,
    lastStep: u32,
    dataPoints: u32,
    chosenSize: ?u32 = null,
    sizeFirstStepCache: *T3.binarySearchFirstHaltingStepForSizeCache,

    prng: *std.Random.DefaultPrng,
    percentIgnore: ?f64 = null,
    sizeStartIgnore: ?u32 = null,
    // sizeTypedHashmap: *exp5sizeTypedHashmapType,
    // sizeTypedTotalsHashmap: *exp5sizeTypedTotalsHashmapType,
};
fn exp5(_: *BigIntManaged, input: []const u8, args: anytype) error{ Yield, Else }!void {
    const args5: exp5args = args;

    const size = T3.getSize(input, args5.allocator) catch return error.Else;

    // const chosenSizew = 5;
    if (args5.chosenSize) |chosenSize| {
        if (size != chosenSize) return;
    }

    if (args5.percentIgnore != null) {
        const random = args5.prng.random().float(f64);
        if (args5.sizeStartIgnore != null) {
            if (args5.sizeStartIgnore.? <= size and random <= args5.percentIgnore.?) return;
        } else {
            if (random <= args5.percentIgnore.?) return;
        }
    }

    var t3Arena = std.heap.ArenaAllocator.init(args5.allocator);
    defer t3Arena.deinit();

    // var haltedCount: u32 = 0;
    // var totalCount: u32 = 0;
    var haltedAlready = false;
    // const returnThing: T3.SizeAndOutput = .{ .inputSize = size, .output = "" };
    // const startStep = (T3.binarySearchFirstHaltingStepForSize(size, args.sizeFirstStepCache, args5.allocator, null, null) catch return error.Else) orelse 1;
    // const startStep = 1;
    // std.debug.print("startStep {}, endStep {}\n", .{ startStep, startStep + args5.maxSteps });
    const stepsTotal = args5.lastStep - args5.startStep;
    const stepIncrement = stepsTotal / args5.dataPoints;
    for (0..args5.dataPoints) |i| {
        const traverses = args5.startStep + i * stepIncrement;
        if (traverses > args5.lastStep) return;

        const result = if (haltedAlready) true else T3.cacaHalted(input, @intCast(traverses), t3Arena.allocator()) catch return error.Else;
        if (result) { //halted
            haltedAlready = true;
            // std.debug.print("{s} halted {} steps\n", .{ resUnwrapped.output, traverses });
            // haltedCount += 1;

            const thing = args5.hashmap.getOrPut(size) catch return error.Else;
            if (!thing.found_existing) {
                thing.value_ptr.* = exp5stepHaltedHashmapType.init(args5.allocator);
            }

            const haltedCountRes = thing.value_ptr.*.getOrPut(@intCast(traverses)) catch return error.Else;
            // haltedCountRes.value_ptr.* = haltedCount;
            if (haltedCountRes.found_existing) {
                haltedCountRes.value_ptr.* += 1;
            } else {
                haltedCountRes.value_ptr.* = 1;
            }
            // std.debug.print("found halting", .{});
        } else {
            // if (size == 5) {
            //     std.debug.print("size {} NOT halted at {} steps\n", .{ size, traverses });
            // }
        }
        const thing = args5.totalsHashmap.getOrPut(size) catch return error.Else;
        if (!thing.found_existing) {
            thing.value_ptr.* = exp5stepHaltedHashmapType.init(args5.allocator);
        }

        // totalCount += 1;
        const totalRes = thing.value_ptr.*.getOrPut(@intCast(traverses)) catch return error.Else;
        // totalRes.value_ptr.* = totalCount;
        if (totalRes.found_existing) {
            totalRes.value_ptr.* += 1;
        } else {
            totalRes.value_ptr.* = 1;
        }
    }

    // if (size == 5) //
    //     std.debug.print("did a size 5\n", .{});
}

const freqHashmapType = std.AutoHashMap(u32, f64);
const freqHashmapTypeType = std.AutoHashMap(u32, *freqHashmapType);

const exp5FitnessContext = struct {
    hashmap: *freqHashmapTypeType,
    chosenSize: u32,
    // startStep: u32,
};

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

var exp5FitnessGlobalContext: exp5FitnessContext = undefined;

//RMSE MAE
pub fn exp5fitness(genome: []BigFloat) f64 {
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
    if (a > 0.01 or b < 0) penaltySum += 100;

    const result = -@sqrt((sum + penaltySum) / n); //RMSE
    // const result = -sum / n; //MAE
    // if (!std.math.isFinite(result)) {
    //     std.debug.print("fitness is not finite!?\n", .{});
    // }
    return result;
    // return -sum;
}

pub fn main(init: std.process.Init) !void {
    std.debug.print("All your {s} are belong to us.\n", .{"codebase"});

    // This is appropriate for anything that lives as long as the process.
    const arena: std.mem.Allocator = init.arena.allocator();

    // try doStuff(arena);

    // {
    //     var a = try BigIntManaged.init(arena);
    //     var b = try BigIntManaged.init(arena);

    //     defer b.deinit();
    //     defer a.deinit();

    //     try a.set(0);
    //     try b.set(20);

    //     try T3.streamBinaryStrsRange(a, b, &exp1_noteOutputs, arena, false);
    // }

    // {
    //     _ = T3.getStepsCapture5Sigma(0.25, 5000);
    // }

    // {
    //     var a = try BigIntManaged.init(arena);
    //     var b = try BigIntManaged.init(arena);

    //     defer b.deinit();
    //     defer a.deinit();

    //     try a.set(0);
    //     try b.set(20);

    //     // const InputAndSize = struct { input: []const u8, size: u32 };
    //     var hashmap = args2hashmap.init(arena);
    //     var args2: exp2Args = .{
    //         .allocator = arena, //
    //         .hashmap = &hashmap,
    //         .arena = std.heap.ArenaAllocator.init(arena),
    //         .traverses = 2000,
    //     };
    //     defer args2.arena.deinit();
    //     defer args2.hashmap.deinit();

    //     // (input, size) => bool

    //     try T3.streamBinaryStrsRange(a, b, &exp2_noteHaltedInputs, args2, false);

    //     std.debug.print("\n", .{});
    // }

    // experiment 3, printing density of sizes
    // this could be improved by iterating the indexes just once and not having a chosenSize
    // var startT3 = now(.awake, init.io);
    // {
    //     const lastSize = 50;

    //     const sizeFile = try std.Io.Dir.cwd().createFile(init.io, "sizes.csv", .{});
    //     defer sizeFile.close(init.io);

    //     var sizeFileBuf: [4000]u8 = undefined;
    //     var sizeFileWriter = sizeFile.writer(init.io, &sizeFileBuf);
    //     const sizeFileWriterPtr = &sizeFileWriter.interface;
    //     defer sizeFileWriter.flush() catch unreachable;

    //     var a = try BigIntManaged.init(arena);
    //     var b = try BigIntManaged.init(arena);
    //     var c = try BigIntManaged.init(arena);
    //     defer a.deinit();
    //     defer b.deinit();
    //     defer c.deinit();

    //     for (0..lastSize) |sizeU| {
    //         const sizez: u32 = @intCast(sizeU);

    //         try c.set(sizez);
    //         _ = try T3.firstIndexSeenForProgramSize(&a, &c, arena);
    //         _ = try T3.lastIndexSeenForProgramSize(&b, &c, arena);

    //         var hashmap = std.AutoHashMap(u32, u32).init(arena);
    //         defer hashmap.deinit();

    //         var indexHashmap = exp3indexHashmap.init(arena);
    //         defer indexHashmap.deinit();

    //         const args: exp3args = .{
    //             .allocator = arena, //
    //             .hashmap = &hashmap,
    //             .indexHashmap = &indexHashmap,
    //             .sizeFileWriter = sizeFileWriterPtr,
    //             .chosenSize = sizez,
    //         };

    //         // std.debug.print("index,size\n", .{});
    //         try T3.streamBinaryStrsRange(a, b, &experiment3, args, false);
    //         // std.debug.print("\n", .{});

    //         const freq = hashmap.get(sizez);

    //         if (freq != null) {
    //             try c.sub(&b, &a);

    //             const indexLength128 = try c.toInt(u128) + 1;

    //             const indexLenBF = BigFloat.init(indexLength128);
    //             const freqBF = BigFloat.init(freq.?);
    //             const densityBF = freqBF.div(indexLenBF);
    //             // std.debug.print("{},{f} a {d}, b {d},freq:{}, indexLengthBF {f}, indexlengtrh128 {}\n", //
    //             //     .{ sizez, densityBF, a, b, freq.?, indexLenBF, indexLength128 });

    //             std.debug.print("{},{f}\n", .{ sizez, densityBF });
    //         }

    //         // var it = hashmap.iterator();
    //         // std.debug.print("\nsize frequencies", .{});
    //         // while (it.next()) |entry| {
    //         //     // if (entry.key_ptr.* > 100) continue;
    //         //     std.debug.print("{},{}\n", .{ entry.key_ptr.*, entry.value_ptr.* });
    //         // }
    //     }

    //     // var it2 = indexHashmap.iterator();
    //     // std.debug.print("\nsize->first seen distribution\n", .{});
    //     // while (it2.next()) |entry| {
    //     //     const value = entry.value_ptr.*;
    //     //     // if (entry.key_ptr.* > 100) continue;
    //     //     std.debug.print("{},{d}\n", .{ entry.key_ptr.*, value.firstSeenIndex.* });
    //     //     // std.debug.print("{},{d}\n", .{ entry.key_ptr.*, value.lastSeenIndex.* });
    //     // }

    //     // var it3 = indexHashmap.iterator();
    //     // std.debug.print("\nsize->last seen distribution\n", .{});
    //     // while (it3.next()) |entry| {
    //     //     const value = entry.value_ptr.*;
    //     //     // if (entry.key_ptr.* > 100) continue;
    //     //     // std.debug.print("{},{d}\n", .{ entry.key_ptr.*, value.firstSeenIndex.* });
    //     //     std.debug.print("{},{d}\n", .{ entry.key_ptr.*, value.lastSeenIndex.* });
    //     // }
    // }
    // std.debug.print("exp3 took {} seconds\n", .{startT3.durationTo(now(.awake, init.io)).toSeconds()});

    //experiment to see if lastSeenIndex and firstSeenIndex work
    // {
    //     var first = try BigIntManaged.init(arena);
    //     var last = try BigIntManaged.init(arena);
    //     var size = try BigIntManaged.init(arena);
    //     defer size.deinit();
    //     defer first.deinit();
    //     defer last.deinit();

    //     const maxSize = 200;
    //     std.debug.print("\n", .{});
    //     for (5..maxSize) |i| {
    //         try size.set(i);

    //         if (try T3.firstIndexSeenForProgramSize(&first, &size, arena) and try T3.lastIndexSeenForProgramSize(&last, &size, arena)) {
    //             std.debug.print("size {} -> first {d}, last {d}\n", .{ i, first, last });
    //         }
    //     }
    // }

    // other experiemtn to see if T3.caca even works...
    // {
    //     for (1..100) |stepsUsize| {
    //         const steps: u32 = @intCast(stepsUsize);

    //         _ = T3.caca("1", steps, arena) catch |err| {
    //             if (err == error.MAX_TRAVERSES) {
    //                 std.debug.print("NOT halted\n", .{});
    //             }
    //             continue;
    //         };

    //         std.debug.print("halted\n", .{});
    //     }
    // }

    //experiment4 estimated max program sizes
    // {
    //     for (1..100) |i| {
    //         std.debug.print("\nsize {}, maxPrograms->{f}", .{ i, try T3.maxEstimatedProgramsOfSize(i) });
    //     }
    // }

    //genetic experiment 1
    var startTg1 = now(.awake, init.io);
    {
        const genomeSize = 2;
        const mutateChance = 0.05;
        const crossoverChance = 0.33;
        const iterations = 1000;
        const populationSize = 1000;
        const selectionTopPercent = 0.2;
        const randomSeed = 42;

        var geneticThing = BFGenetic.BFVectorEvolutionContext(genomeSize, BFGenetic.fitnessTestFunc3, mutateChance, crossoverChance, iterations, populationSize, selectionTopPercent, randomSeed){};

        geneticThing.epoch();

        std.debug.print("top fitness individual: a {f}, b {f}, fitness {}\n", .{ //
            geneticThing.getPopulation()[0].genome[0],
            geneticThing.getPopulation()[0].genome[1],
            geneticThing.getPopulation()[0].fitness,
        });
    }
    std.debug.print("genetic experiment1 took {} seconds\n", .{startTg1.durationTo(now(.awake, init.io)).toSeconds()});

    //experiemnt 5, cumulative percent halted distribution for a size, x=steps y=cumulativePercentHalted
    var startT = now(.awake, init.io);
    {
        const firstStepExp5 = 33;
        const lastStepExp5 = 59;
        const percentHaltedEnding = 0.9999;
        const percentIgnore = 0.0;

        for ((firstStepExp5 - 1) / 2 + 1..(lastStepExp5 - 1) / 2 + 1) |i| {
            const chosenFreqSize: u32 = 2 * @as(u32, @intCast(i)) + 1;
            if (T3.sizeGetTypeDivTen(chosenFreqSize) == null) continue;
            var hashmap = exp5hashmapType.init(arena);
            defer hashmap.deinit();

            var totalsHashmap = exp5hashmapType.init(arena);
            defer totalsHashmap.deinit();

            var sizeFirstCache = T3.binarySearchFirstHaltingStepForSizeCache.init(arena);
            defer sizeFirstCache.deinit();

            var a = try BigIntManaged.init(arena);
            var b = try BigIntManaged.init(arena);
            defer b.deinit();
            defer a.deinit();

            // const chosenFreqSize = 53;

            // const dataPoints = 20;
            //TODO add middlePoints and endStartPoints to help stabilize fitting
            // do this if TODO fails: make a step "schedule" where 80% points are at the ends and 20% is in the "middle"

            // const sizeStartIgnore = null;

            const randomSeed = 42;

            const startStep = try T3.binarySearchFirstHaltingStepForSize(chosenFreqSize, &sizeFirstCache, arena, percentIgnore, randomSeed, &hashmap, &totalsHashmap) orelse 1;
            const lastStep = (try T3.sampledBinarySearchNPercentHaltingStepForSize(chosenFreqSize, percentHaltedEnding, null, arena, percentIgnore, randomSeed, &hashmap, &totalsHashmap) orelse startStep + 500) + 1;

            var bigSize = try BigIntManaged.init(arena);
            defer bigSize.deinit();

            // if (chosenFreqSize != null) {
            try bigSize.set(chosenFreqSize);

            _ = try T3.firstIndexSeenForProgramSize(&a, &bigSize, arena);
            _ = try T3.lastIndexSeenForProgramSize(&b, &bigSize, arena);
            // }

            // var prng: std.Random.DefaultPrng = .init(42);
            // const args: exp5args = .{
            //     .allocator = arena, //
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
            var frequencyHashmap = freqHashmapTypeType.init(arena);
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
                    for (startStep..lastStep) |stepsUsize| {
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
                                const newHashmap = try arena.create(freqHashmapType);
                                newHashmap.* = freqHashmapType.init(arena);

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
            {
                const sizeFile = try std.Io.Dir.cwd().createFile(init.io, "sizeHaltingFrequencies.txt", .{});
                defer sizeFile.close(init.io);

                var sizeFileBuf: [4000]u8 = undefined;
                var sizeFileWriter = sizeFile.writer(init.io, &sizeFileBuf);
                const sizeFileWriterPtr = &sizeFileWriter.interface;
                defer sizeFileWriter.flush() catch unreachable;

                // var lastPercentHalted: f64 = 0.0;
                var it2 = frequencyHashmap.iterator();
                while (it2.next()) |entry| {
                    const size = entry.key_ptr.*;
                    try sizeFileWriterPtr.print("\n\nsize {}:\n", .{size});
                    const freqMap = entry.value_ptr.*;
                    // const startStep = try T3.binarySearchFirstHaltingStepForSize(size, &sizeFirstCache, arena, percentIgnore, null) orelse 1;
                    for (startStep..lastStep) |steps| {
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
            std.debug.print("size: {}\n", .{chosenFreqSize});
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

                std.debug.print("\ttop fitness individual: a {f}, b {f}, fitness {}\n", .{ //
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

                const endingSteps = (varA.mul(varB).sub(BigFloat.log2(oneBF.div(desiredProbBF).sub(oneBF)).div(BigFloat.log2(eF)))).div(varA);

                const endingStepsInt: u32 = if (chosenFreqSize < 40 and hundredPercentHaltingStep != null) //
                    @intFromFloat(@as(f64, @floatFromInt(hundredPercentHaltingStep.?)) * 1.3) // increase 30% i suppose
                else
                    @intFromFloat(endingSteps.toFloat(f64));
                // const endingStepsInt: u32 = std.math.lossyCast(u32, endingSteps.toFloat(f64));

                std.debug.print("\tsteps to achieve {}% halting is ~{}\n", .{ percentHaltedEnding * 100, endingStepsInt });
            }

            // std.debug.print("\n", .{});
        }
    }
    std.debug.print("exp5 took {} seconds\n", .{startT.durationTo(now(.awake, init.io)).toSeconds()});

    //experiejmnt 6,
    // var startT6 = now(.awake, init.io);
    // {
    //     const startSize = 0;
    //     const lastSize = 55;
    //     const percentIgnore = 0.0;
    //     for (startSize..lastSize) |chosenSizeUsize| {
    //         const haltedStepMaybe = try T3.binarySearchFirstHaltingStepForSize(@intCast(chosenSizeUsize), null, arena, percentIgnore, null);
    //         if (haltedStepMaybe) |haltedStep| {
    //             std.debug.print("{},{}\n", .{ chosenSizeUsize, haltedStep });
    //         }
    //     }
    //     std.debug.print("\n", .{});
    // }
    // std.debug.print("exp6 took {} seconds\n", .{startT6.durationTo(now(.awake, init.io)).toSeconds()});

    //experiejmnt 7, binary search 99% halting step
    // var startT7 = now(.awake, init.io);
    // {
    //     var type1Msg: std.ArrayList(u8) = .empty;
    //     var type2Msg: std.ArrayList(u8) = .empty;
    //     defer type1Msg.deinit(arena);
    //     defer type2Msg.deinit(arena);

    //     const startSize = 0;
    //     const lastSize = 55;
    //     const atLeastPercentHalted: f64 = 0.99;
    //     const percentIgnore = 0.8;
    //     for (startSize..lastSize) |chosenSizeUsize| {
    //         const haltedStepMaybe = try T3.sampledBinarySearchNPercentHaltingStepForSize(@intCast(chosenSizeUsize), atLeastPercentHalted, null, arena, percentIgnore, null);
    //         if (haltedStepMaybe) |haltedStep| {
    //             const sizeType = T3.sizeGetTypeDivFour(@intCast(chosenSizeUsize));
    //             if (sizeType != null) {
    //                 const msg = try std.fmt.allocPrint(arena, "{},{}\n", .{ chosenSizeUsize, haltedStep });
    //                 switch (sizeType.?) {
    //                     T3.SizeTypeDivFour.MINUS_ONE => {
    //                         try type1Msg.appendSlice(arena, msg);
    //                     },
    //                     T3.SizeTypeDivFour.MINUS_THREE => {
    //                         try type2Msg.appendSlice(arena, msg);
    //                     },
    //                 }
    //             }
    //         }
    //     }
    //     std.debug.print("{s}\n{s}\n", .{ type1Msg.items, type2Msg.items });
    // }
    // const endT7 = now(.awake, init.io);
    // std.debug.print("exp7 took {} seconds\n", .{startT7.durationTo(endT7).toSeconds()});

    //TODO see "new 12" and make a experiment that reproduces it faster than 30 min, and also find more size data points.
    // see previous experiments, skip the machinesn sizes that have halted already idk bruv

    // Accessing command line arguments:
    const args = try init.minimal.args.toSlice(arena);
    for (args) |arg| {
        std.log.info("arg: {s}", .{arg});
    }

    // In order to do I/O operations need an `Io` instance.
    const io = init.io;

    // Stdout is for the actual output of your application, for example if you
    // are implementing gzip, then only the compressed bytes should be sent to
    // stdout, not any debugging messages.
    var stdout_buffer: [1024]u8 = undefined;
    var stdout_file_writer: Io.File.Writer = .init(.stdout(), io, &stdout_buffer);
    const stdout_writer = &stdout_file_writer.interface;

    try turing3.printAnotherMessage(stdout_writer);

    try stdout_writer.flush(); // Don't forget to flush!
}

test "simple test" {
    const gpa = std.testing.allocator;
    var list: std.ArrayList(i32) = .empty;
    defer list.deinit(gpa); // Try commenting this out and see if zig detects the memory leak!
    try list.append(gpa, 42);
    try std.testing.expectEqual(@as(i32, 42), list.pop());
}

test "fuzz example" {
    try std.testing.fuzz({}, testOne, .{});
}

fn testOne(context: void, smith: *std.testing.Smith) !void {
    _ = context;
    // Try passing `--fuzz` to `zig build test` and see if it manages to fail this test case!

    const gpa = std.testing.allocator;
    var list: std.ArrayList(u8) = .empty;
    defer list.deinit(gpa);
    while (!smith.eos()) switch (smith.value(enum { add_data, dup_data })) {
        .add_data => {
            const slice = try list.addManyAsSlice(gpa, smith.value(u4));
            smith.bytes(slice);
        },
        .dup_data => {
            if (list.items.len == 0) continue;
            if (list.items.len > std.math.maxInt(u32)) return error.SkipZigTest;
            const len = smith.valueRangeAtMost(u32, 1, @min(32, list.items.len));
            const off = smith.valueRangeAtMost(u32, 0, @intCast(list.items.len - len));
            try list.appendSlice(gpa, list.items[off..][0..len]);
            try std.testing.expectEqualSlices(
                u8,
                list.items[off..][0..len],
                list.items[list.items.len - len ..],
            );
        },
    };
}
