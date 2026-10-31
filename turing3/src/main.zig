const std = @import("std");
const Io = std.Io;

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
};
fn experiment3(index: *BigIntManaged, input: []const u8, argz: anytype) error{ Yield, Else }!void {
    const args: exp3args = argz;

    var theHashmap = args.hashmap;

    const size = T3.getSize(input, args.allocator) catch {
        return error.Else;
    };

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
    maxSteps: u32,
    chosenSize: ?u32,
    sizeFirstStepCache: *T3.binarySearchFirstHaltingStepForSizeCache,
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

    var t3Arena = std.heap.ArenaAllocator.init(args5.allocator);
    defer t3Arena.deinit();

    // var haltedCount: u32 = 0;
    // var totalCount: u32 = 0;
    var haltedAlready = false;
    // const returnThing: T3.SizeAndOutput = .{ .inputSize = size, .output = "" };
    const startStep = (T3.binarySearchFirstHaltingStepForSize(size, args.sizeFirstStepCache, args5.allocator) catch return error.Else) orelse 1;
    // const startStep = 1;
    std.debug.print("startStep {}, endStep {}\n", .{ startStep, startStep + args5.maxSteps });
    for (startStep..(startStep + args5.maxSteps)) |traverses| {
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

pub fn main(init: std.process.Init) !void {
    std.debug.print("All your {s} are belong to us.\n", .{"codebase"});

    // This is appropriate for anything that lives as long as the process.
    const arena: std.mem.Allocator = init.arena.allocator();

    try doStuff(arena);

    {
        var a = try BigIntManaged.init(arena);
        var b = try BigIntManaged.init(arena);

        defer b.deinit();
        defer a.deinit();

        try a.set(0);
        try b.set(20);

        try T3.streamBinaryStrsRange(a, b, &exp1_noteOutputs, arena, false);
    }

    {
        _ = T3.getStepsCapture5Sigma(0.25, 5000);
    }

    {
        var a = try BigIntManaged.init(arena);
        var b = try BigIntManaged.init(arena);

        defer b.deinit();
        defer a.deinit();

        try a.set(0);
        try b.set(20);

        // const InputAndSize = struct { input: []const u8, size: u32 };
        var hashmap = args2hashmap.init(arena);
        var args2: exp2Args = .{
            .allocator = arena, //
            .hashmap = &hashmap,
            .arena = std.heap.ArenaAllocator.init(arena),
            .traverses = 2000,
        };
        defer args2.arena.deinit();
        defer args2.hashmap.deinit();

        // (input, size) => bool

        try T3.streamBinaryStrsRange(a, b, &exp2_noteHaltedInputs, args2, false);

        std.debug.print("\n", .{});
    }

    // experiment 3, collecting frequencies of program sizes
    // {
    //     var a = try BigIntManaged.init(arena);
    //     var b = try BigIntManaged.init(arena);

    //     defer b.deinit();
    //     defer a.deinit();

    //     try a.set(0);
    //     try b.set(1_000_000);

    //     var hashmap = std.AutoHashMap(u32, u32).init(arena);
    //     defer hashmap.deinit();

    //     var indexHashmap = exp3indexHashmap.init(arena);
    //     defer indexHashmap.deinit();

    //     const sizeFile = try std.Io.Dir.cwd().createFile(init.io, "sizes.csv", .{});
    //     defer sizeFile.close(init.io);

    //     var sizeFileBuf: [4000]u8 = undefined;
    //     var sizeFileWriter = sizeFile.writer(init.io, &sizeFileBuf);
    //     const sizeFileWriterPtr = &sizeFileWriter.interface;
    //     defer sizeFileWriter.flush() catch unreachable;

    //     const args: exp3args = .{
    //         .allocator = arena, //
    //         .hashmap = &hashmap,
    //         .indexHashmap = &indexHashmap,
    //         .sizeFileWriter = sizeFileWriterPtr,
    //     };

    //     std.debug.print("index,size\n", .{});
    //     try T3.streamBinaryStrsRange(a, b, &experiment3, args);
    //     std.debug.print("\n", .{});

    //     var it = hashmap.iterator();
    //     std.debug.print("\nsize frequencies", .{});
    //     while (it.next()) |entry| {
    //         // if (entry.key_ptr.* > 100) continue;
    //         std.debug.print("{},{}\n", .{ entry.key_ptr.*, entry.value_ptr.* });
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

    //experiemnt 5
    var startT = std.Io.Clock.now(.awake, init.io);
    {
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

        //curve 1 x length, 5,7     5,9
        // const maxSteps = 9;
        // // const chosenFreqSize = 5;
        // try a.set(0);
        // try b.set(1_000);

        //curve 1 x length, 19,22     19,27
        // const maxSteps = 250;
        // const chosenFreqSize = 19;
        // try a.set(0);
        // try b.set(100_000);

        //curve 1 x length, 25,25     25,39
        // const maxSteps = 80;
        // const chosenFreqSize = 25;
        // try a.set(0);
        // try b.set(100_000);

        // curve 6 x length, 31,40       31,48
        // const maxSteps = 80;
        // const chosenFreqSize = 31;
        // try a.set(0);
        // try b.set(100_000);

        // curve 43 x length, 55,40     55,84
        // const maxSteps = 950;
        // const chosenFreqSize = 55;
        // try a.set(0);
        // try b.set(200_000);

        // const maxSteps = 950;
        // const chosenFreqSize = 59;
        // try a.set(0);
        // try b.set(200_000);

        // curve 62 x length, 73,48       73,111
        // const maxSteps = 1250;
        // const chosenFreqSize = 73;
        // try a.set(0);
        // try b.set(200_000);

        // 78 curve x length, 87,53        87,132
        // const maxSteps = 132;
        // const chosenFreqSize = 87;
        // try a.set(0);
        // try b.set(200_000);

        // 80 curve x length, 91,57        91,138
        // const maxSteps = 138;
        // const chosenFreqSize = 91;
        // try a.set(0);
        // try b.set(300_000);

        // 91 curve x length, 97,55      97,147
        // const maxSteps = 147;
        // // const chosenFreqSize = 97;
        // try a.set(0);
        // try b.set(300_000);

        // lastSteps = 1.5*size + 1.5

        // const chosenFreqSize = null;

        const chosenFreqSize = 39;
        const maxSteps = 700;

        var bigSize = try BigIntManaged.init(arena);
        defer bigSize.deinit();

        // if (chosenFreqSize != null) {
        try bigSize.set(chosenFreqSize);

        _ = try T3.firstIndexSeenForProgramSize(&a, &bigSize, arena);
        _ = try T3.lastIndexSeenForProgramSize(&b, &bigSize, arena);
        // }

        const args: exp5args = .{
            .allocator = arena, //
            .hashmap = &hashmap,
            .totalsHashmap = &totalsHashmap,
            .maxSteps = maxSteps,
            .chosenSize = chosenFreqSize,
            .sizeFirstStepCache = &sizeFirstCache,
        };

        try T3.streamBinaryStrsRange(a, b, &exp5, args, true);

        // size -> (steps -> freq)
        const freqHashmapType = std.AutoHashMap(u32, f64);
        var frequencyHashmap = std.AutoHashMap(u32, *freqHashmapType).init(arena);
        defer frequencyHashmap.deinit();

        var it = totalsHashmap.iterator();
        std.debug.print("\n", .{});
        while (it.next()) |entry| {
            const size = entry.key_ptr.*;

            // const totalCount = entry.value_ptr.*;

            const stepHaltedHashmapMaybe = hashmap.get(size);
            const stepTotalHashmapMaybe = totalsHashmap.get(size);

            std.debug.print("\n\n size {} distribution:\n", .{size});
            if (stepHaltedHashmapMaybe != null and stepTotalHashmapMaybe != null) {
                const stepHaltedHashmap = stepHaltedHashmapMaybe.?;
                const stepTotalHashmap = stepTotalHashmapMaybe.?;

                // var haltedCounter: u32 = 0;
                // const totalTotalCount = totalCount * maxSteps;

                const startStep = try T3.binarySearchFirstHaltingStepForSize(size, &sizeFirstCache, arena) orelse 1;
                // const startStep = 1;
                for (startStep..(startStep + maxSteps)) |stepsUsize| {
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

                        if (size == chosenFreqSize) //
                            std.debug.print("{} halted out of {} in {} steps\n", .{ haltedCount, total, steps });
                    }

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

        {
            const sizeFile = try std.Io.Dir.cwd().createFile(init.io, "sizeHaltingFrequencies.txt", .{});
            defer sizeFile.close(init.io);

            var sizeFileBuf: [4000]u8 = undefined;
            var sizeFileWriter = sizeFile.writer(init.io, &sizeFileBuf);
            const sizeFileWriterPtr = &sizeFileWriter.interface;
            defer sizeFileWriter.flush() catch unreachable;

            var lastPercentHalted: f64 = 0.0;
            var it2 = frequencyHashmap.iterator();
            while (it2.next()) |entry| {
                const size = entry.key_ptr.*;
                try sizeFileWriterPtr.print("\n\nsize {}:\n", .{size});
                const freqMap = entry.value_ptr.*;
                const startStep = try T3.binarySearchFirstHaltingStepForSize(size, &sizeFirstCache, arena) orelse 1;
                for (startStep..(startStep + maxSteps)) |steps| {
                    const percentHaltedMaybe: ?f64 = freqMap.get(@intCast(steps));

                    if (percentHaltedMaybe) |percentHalted| {
                        try sizeFileWriterPtr.print("{},{}\n", .{ steps, percentHalted });
                        lastPercentHalted = percentHalted;
                    } else {
                        try sizeFileWriterPtr.print("{},{}\n", .{ steps, lastPercentHalted });
                    }
                }
            }
        }
        std.debug.print("\n", .{});
    }
    const endT = std.Io.Clock.now(.awake, init.io);
    std.debug.print("exp5 took {} seconds\n", .{startT.durationTo(endT).toSeconds()});

    // TODO consolidate exp6 into a function like "T3.binarySearchFirstHaltingStepForSize(size) -> ?u32"
    // we are going to want to use this back in experiemnt5 so instead of starting at 0 we start at the
    // first step that gives us halting machines.

    // TODO we make a new experiment that tries to estimate the number of programs better and use that to calculate
    // if we are capturing enough machines to call it "good enough" sampling.

    // TODO experiment5 should be modified so it tries to take N data points and not all the steps from A to B.

    // question? when do we stop sampling????

    //experiejmnt 6,
    var startT6 = std.Io.Clock.now(.awake, init.io);
    {
        const startSize = 0;
        const lastSize = 55;
        for (startSize..lastSize) |chosenSizeUsize| {
            const haltedStepMaybe = try T3.binarySearchFirstHaltingStepForSize(@intCast(chosenSizeUsize), null, arena);
            if (haltedStepMaybe) |haltedStep| {
                std.debug.print("{},{}\n", .{ chosenSizeUsize, haltedStep });
            }
        }
        std.debug.print("\n", .{});
    }
    const endT6 = std.Io.Clock.now(.awake, init.io);
    std.debug.print("exp6 took {} seconds\n", .{startT6.durationTo(endT6).toSeconds()});

    //experiejmnt 7, binary search 99% halting step
    var startT7 = std.Io.Clock.now(.awake, init.io);
    {
        const startSize = 0;
        const lastSize = 55;
        for (startSize..lastSize) |chosenSizeUsize| {
            const haltedStepMaybe = try T3.binarySearch99PercentHaltingStepForSize(@intCast(chosenSizeUsize), null, arena);
            if (haltedStepMaybe) |haltedStep| {
                std.debug.print("{},{}\n", .{ chosenSizeUsize, haltedStep });
            }
        }
        std.debug.print("\n", .{});
    }
    const endT7 = std.Io.Clock.now(.awake, init.io);
    std.debug.print("exp6 took {} seconds\n", .{startT7.durationTo(endT7).toSeconds()});

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
