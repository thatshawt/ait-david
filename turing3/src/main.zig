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

const mixedRadixLoadExprToSlice = @import("MixedRadixTerms.zig").mixedRadixLoadExprToSlice;
const ArithmeticCoder = @import("BigIntArithmeticCoding.zig").BigIntRangeCoder;

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

    // var formatBuff: [100]u8 = undefined;

    // var stdout_writer = std.Io.File.writer(std.Io.File.stdout(), std.Io, stdoutBuff);

    //testing randomBigIntRange
    // {
    //     var rng = std.Random.DefaultPrng.init(42);
    //     var randomVal = try BigIntManaged.init(allocator);
    //     var first = try BigIntManaged.initSet(allocator, -10);
    //     var last = try BigIntManaged.initSet(allocator, 10);
    //     defer {
    //         randomVal.deinit();
    //         first.deinit();
    //         last.deinit();
    //     }
    //     for (0..100) |_| {
    //         try T3.randomBigIntRange(&randomVal, &first, &last, &rng);
    //         std.debug.print("{d} ", .{randomVal});
    //     }
    //     std.debug.print("\n", .{});
    // }

    //big integer percent test
    // {
    //     var result = try BigIntManaged.init(allocator);
    //     defer result.deinit();

    //     var number = try BigIntManaged.initSet(allocator, 123);
    //     defer number.deinit();
    //     const percent: f64 = 0.511;
    //     const precision: u32 = 3;

    //     try T3.bigIntPercent(&result, &number, precision, percent, allocator);

    //     std.debug.print("%{} of {d} is {d}\n", .{ percent, number, result });
    // }

    // unique random number test
    // {
    //     var randomVal = try BigIntManaged.init(allocator);
    //     var first = try BigIntManaged.initSet(allocator, 10);
    //     var last = try BigIntManaged.initSet(allocator, 31);
    //     var rng: T3.FeistelCipher = undefined;
    //     try rng.init(allocator, &first, &last, 42);
    //     defer {
    //         randomVal.deinit();
    //         first.deinit();
    //         last.deinit();
    //         rng.deinit();
    //     }
    //     for (0..25) |_| {
    //         try rng.loadNext(&randomVal);
    //         std.debug.print("{d} ", .{randomVal});
    //     }
    //     std.debug.print("\n", .{});
    //     return;
    // }

    // cantor pair
    var cantorResult = try BigIntManaged.init(allocator);
    defer cantorResult.deinit();

    var a = try BigIntManaged.init(allocator);
    var b = try BigIntManaged.init(allocator);
    defer a.deinit();
    defer b.deinit();

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

    const iMax = 300;
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

        const thing = try generate(allocator, &i);
        // std.debug.print("   ",.{});

        // const blcSliceString = try loadBlcExpressionToSlice(thing, allocator);
        // const blcSliceString = try mixedRadixLoadExprToSlice(thing, allocator);
        const blcSliceString = ArithmeticCoder.encode(thing, allocator) catch continue;
        std.debug.print("{}\n\t=> \"{s}\"\n\t\t=generateExpression> ", .{ iInt, fmtBuffer });
        thing.traversePrint();
        std.debug.print("\n\t\t=blc> ", .{});
        std.debug.print("\"{s}\"", .{blcSliceString});

        try i.setString(2, blcSliceString);
        const blcNumber = try i.toString(allocator, 10, std.fmt.Case.lower);
        defer allocator.free(blcNumber);
        std.debug.print("\n\t\t=> {s}", .{blcNumber});

        // std.debug.print("\n\n\t\t=jotParse> ", .{});
        const jotExpr = try jotParser.parse(binarySlice);
        // jotExpr.traversePrint();

        // std.debug.print("\n\t\t\t=size> {}", .{jotExpr.size()});

        if (interpreter.reduceUntilCantReduce(jotExpr, 100, null, true)) |_| {
            // std.debug.print("\n\t\t=reduce> ", .{});
            // reducedExpr.traversePrint();
            // const blcSliceString2 = try loadBlcExpressionToSlice(reducedExpr, allocator);
            // const blcSliceString2 = try mixedRadixLoadExprToSlice(reducedExpr, allocator);
            // std.debug.print("\n\t\t=blc> ", .{});
            // std.debug.print("\"{s}\"", .{blcSliceString2});
        } else |err| switch (err) {
            else => {
                // std.debug.print("\n\t\t=reduce> {}", .{err});
            },
        }

        std.debug.print("\n\n", .{});
    }

    var builder: LambdaBuilder = .{ .arena = std.heap.ArenaAllocator.init(allocator) };
    (try builder.apply(.{ .expr = try builder.apply(.{ .text = "999999" }, .{ .text = "/(/(/(((3 1) (2 1)))))" }) }, .{ .text = "/(/(2))" })).traversePrint();
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

// const exp5sizeTypedHashmapType = std.AutoHashMap(exp5SizeAndType, exp5stepHaltedHashmapType);

// const exp5AndType = struct { size: u32, sType: T3.SizeTypeDivTen };
// const exp5sizeTypedTotalsHashmapType = std.AutoHashMap(exp5SizeAndType, exp5stepHaltedHashmapType);
const exp5args = struct {
    allocator: std.mem.Allocator, //
    hashmap: *T3.exp5hashmapType,
    totalsHashmap: *T3.exp5hashmapType, // size -> <steps -> total>
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
                thing.value_ptr.* = T3.exp5stepHaltedHashmapType.init(args5.allocator);
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
            thing.value_ptr.* = T3.exp5stepHaltedHashmapType.init(args5.allocator);
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

var exp8GlobalConstBigOne: ?BigIntManaged = null;
const exp8OutputFreqMapType = std.StringHashMap(BigIntManaged);
// const exp8TotalsMapType = std.AutoHashMap(, );
const exp8args = struct {
    allocator: std.mem.Allocator, //
    interpreterAllocator: *std.heap.FixedBufferAllocator,

    chosenSize: ?u32 = null,
    traverses: u32,

    // str -> u32
    outputFreqMap: *exp8OutputFreqMapType,

    prng: *std.Random.DefaultPrng,
    percentIgnore: ?f64,
};
fn exp8(_: *BigIntManaged, input: []const u8, argsz: anytype) error{ Yield, Else }!void {
    const args: exp8args = argsz;

    if (exp8GlobalConstBigOne == null) exp8GlobalConstBigOne = //
        BigIntManaged.initSet(args.allocator, 1) catch return error.Else;

    const size = T3.getSize(input, args.allocator) catch return error.Else;
    if (args.chosenSize != null and args.chosenSize.? != size) return;

    if (args.percentIgnore != null) {
        const random = args.prng.random().float(f64);

        if (random <= args.percentIgnore.?) return;
    }

    args.interpreterAllocator.reset();

    // run the machine
    const sizeAndOutput = T3.caca2(input, args.traverses, args.interpreterAllocator.allocator()) catch return error.Else;

    // did not halt
    if (sizeAndOutput == null) {
        return;
    }

    // allocate an output with args.allocator, not args.interpreterAllocator which resets very often
    const stableOutput = args.allocator.alloc(u8, sizeAndOutput.?.output.len) catch return error.Else;
    @memcpy(stableOutput, sizeAndOutput.?.output);

    const getResult = args.outputFreqMap.getOrPut(stableOutput) catch return error.Else;
    if (getResult.found_existing) {
        getResult.value_ptr.*.add(getResult.value_ptr, &exp8GlobalConstBigOne.?) catch return error.Else;
    } else {
        getResult.value_ptr.* = BigIntManaged.initSet(args.allocator, 1) catch return error.Else;
    }
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
    // var startT = now(.awake, init.io);
    // {
    //     const percentHaltedEnding = 0.9999;
    //     const percentIgnore = 0.0; //this might stay 0.0 for now
    //     const randomSeed = 42;

    //     const firstStepExp5 = 41;
    //     const lastStepExp5 = 45;

    //     for ((firstStepExp5 - 1) / 2..(lastStepExp5 - 1) / 2 + 1) |i| {
    //         const chosenFreqSize: u32 = 2 * @as(u32, @intCast(i)) + 1;
    //         if (T3.sizeGetTypeDivTen(chosenFreqSize) == null) continue;

    //         const haltingStep = try T3.sampleEstimateHaltingNStep(chosenFreqSize, percentHaltedEnding, percentIgnore, randomSeed, arena, init.io);
    //         if (haltingStep != null) {
    //             std.debug.print("size {}: steps to achieve {}% halting is ~{}\n", .{ chosenFreqSize, percentHaltedEnding * 100, haltingStep.? });
    //         }
    //     }
    // }
    // std.debug.print("exp5 took {} seconds\n", .{startT.durationTo(now(.awake, init.io)).toSeconds()});

    // experiejmnt 6,
    // var startT6 = now(.awake, init.io);
    // {
    //     const startSize = 51;
    //     const lastSize = 65;
    //     const percentIgnore = 0.8;
    //     const randomSeed: u64 = 42;
    //     const percentHalting = 0.9999;
    //     const maxInterpreterMemoryBytes = 1024 * 1024 * 100; // 100Mb

    //     for (startSize..lastSize) |chosenSizeUsize| {
    //         // const chosenSize = 85;
    //         const chosenSize: u32 = @intCast(chosenSizeUsize);

    //         const stepAndMem = try T3.estimateStepAndMemoryForSizeNPercentHalting(chosenSize, percentHalting, percentIgnore, randomSeed, maxInterpreterMemoryBytes, arena);

    //         if (stepAndMem != null) {
    //             std.debug.print("{},{},{}\n", .{ chosenSizeUsize, stepAndMem.?.step, stepAndMem.?.mem });
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

    //experiment 8,
    var startT8 = now(.awake, init.io);
    {
        const chosenSize = 85;
        const percentHalting = 0.9999;
        const randomSeed = 42;
        const percentIgnoreSample = 0.8;
        const percentIgnoreEnumeration = 0.0;

        const maxInterpreterMemoryBytes = 1024 * 1024 * 500; // 500Mb
        // const sizeFile = try std.Io.Dir.cwd().createFile(init.io, "sizeHaltingFrequencies.txt", .{});

        // const haltingStep = try T3.sampleEstimateHaltingNStep(chosenSize, percentHalting, percentIgnoreSample, randomSeed, arena, maxInterpreterMemoryBytes, init.io, sizeFile);

        const stepAndMem = try T3.estimateStepAndMemoryForSizeNPercentHalting(chosenSize, percentHalting, percentIgnoreSample, randomSeed, maxInterpreterMemoryBytes, arena);

        if (stepAndMem == null) {
            std.debug.print("could not determine steps and mem!\n", .{});
            return;
        }

        var freqMap: exp8OutputFreqMapType = exp8OutputFreqMapType.init(arena);
        var first = try BigIntManaged.init(arena);
        var last = try BigIntManaged.init(arena);
        var bigSize = try BigIntManaged.initSet(arena, chosenSize);

        defer {
            freqMap.deinit();
            first.deinit();
            last.deinit();
            bigSize.deinit();
        }

        _ = try T3.firstIndexSeenForProgramSize(&first, &bigSize, arena);
        _ = try T3.lastIndexSeenForProgramSize(&last, &bigSize, arena);

        try first.set(0);

        var prng = std.Random.DefaultPrng.init(randomSeed);

        const interprerBuffer = try arena.alloc(u8, stepAndMem.?.mem);
        defer arena.free(interprerBuffer);
        var interpreterAllocator = std.heap.FixedBufferAllocator.init(interprerBuffer);

        var args: exp8args = .{
            .allocator = arena,
            .interpreterAllocator = &interpreterAllocator,
            .traverses = stepAndMem.?.step,
            // .chosenSize = chosenSize,
            .outputFreqMap = &freqMap,
            .percentIgnore = percentIgnoreEnumeration,
            .prng = &prng,
        };
        std.debug.print("{} steps, {} mem, indedices:{d},{d}\n", .{ stepAndMem.?.step, stepAndMem.?.mem, first, last });

        args.percentIgnore = 0.0;
        try T3.streamUniqueRandomBinaryStrsRange(arena, first, last, 1.0 - percentIgnoreEnumeration, randomSeed, exp8, args, false);

        // try T3.streamBinaryStrsRange(first, last, exp8, args, false);

        // var it = freqMap.iterator();
        // while (it.next()) |entry| {
        //     const output = entry.key_ptr.*;
        //     const freq = entry.value_ptr.*;
        //     std.debug.print("{s},{d}\n", .{ output, freq });
        // }

        // 2. Define an entry struct and collect items from the map
        const Entry = struct {
            key: []const u8,
            value: BigIntManaged,
        };

        var entries = try arena.alloc(Entry, freqMap.count());
        defer arena.free(entries);

        var it = freqMap.iterator();
        var i: usize = 0;
        while (it.next()) |kv| : (i += 1) {
            entries[i] = .{
                .key = kv.key_ptr.*,
                .value = kv.value_ptr.*,
            };
        }

        // 3. Sort the slice by string length (ascending)
        std.mem.sort(Entry, entries, {}, struct {
            fn lessThan(_: void, lhs: Entry, rhs: Entry) bool {
                return lhs.key.len < rhs.key.len;
            }
        }.lessThan);

        var totalSum = try BigIntManaged.initSet(arena, 0);
        defer totalSum.deinit();

        // 4. Print the sorted entries
        for (entries) |entry| {
            std.debug.print("{s},{d}\n", .{ entry.key, entry.value });
            try totalSum.add(&totalSum, &entry.value);
        }

        std.debug.print("total machines {d}\n", .{totalSum});
    }
    std.debug.print("exp8 took {} seconds\n", .{startT8.durationTo(now(.awake, init.io)).toSeconds()});

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
