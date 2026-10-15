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

fn streamBinaryStrsRange(
    // allocator: std.mem.Allocator, //
    a: BigIntManaged, //
    b: BigIntManaged, //
    comptime foreach: *const fn ([]const u8, args: anytype) error{ Yield, Else }!void, //
    args: anytype, //
) !void {
    var fmtBuffer: [400]u8 = undefined;

    var i: BigIntManaged = try a.clone();
    defer i.deinit();

    var onwards = true;
    while (onwards) {
        @memset(&fmtBuffer, 0);

        const binarySlice = try std.fmt.bufPrint(&fmtBuffer, "{b}", .{i});

        // std.debug.print("\n{s}", .{binarySlice});

        try foreach(binarySlice, args);

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

fn exp1_noteOutputs(input: []const u8, args: anytype) error{ Yield, Else }!void {
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
fn exp2_noteHaltedInputs(input: []const u8, args: anytype) error{ Yield, Else }!void {
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

const experiment3args = struct {
    allocator: std.mem.Allocator, //
    hashmap: *std.AutoHashMap(u32, u32),
};
fn experiment3(input: []const u8, args: anytype) error{ Yield, Else }!void {
    var theHashmap = @as(*std.AutoHashMap(u32, u32), args.hashmap);

    const size = T3.getSize(input, args.allocator) catch {
        return error.Else;
    };

    // std.debug.print("\n{s} size {} =>, {s}", .{ input, result.inputSize, result.output });

    const thing = theHashmap.getOrPut(size) catch return error.Else;
    if (thing.found_existing) {
        thing.value_ptr.* += 1;
    } else {
        thing.value_ptr.* = 1;
    }
}

//TODO make exp5
//binaryStream(a,b, foreach = {
//  for(1..maxSteps) T3.caca(binaryStr, steps) ->
//      if(halted) haltedHashmap.getPut(size, sizeValue++)
//      finally totalsHashmap.getPut(size, sizeValue++)
//})
const exp5args = struct {
    allocator: std.mem.Allocator, //
    hashmap: *std.AutoHashMap(u32, u32),
    totalsHashmap: *std.AutoHashMap(u32, u32),
    maxSteps: u32,
};
fn exp5(input: []const u8, args: anytype) error{ Yield, Else }!void {
    const args5: exp5args = args;

    const size = T3.getSize(input, args5.allocator) catch return error.Else;

    for (1..args5.maxSteps) |traverses| {
        const result = T3.caca(input, @intCast(traverses), args5.allocator) catch |err| lbl: {
            if (err == error.MAX_TRAVERSES) {
                const returnThing: T3.SizeAndOutput = .{ .inputSize = size, .output = "" };
                break :lbl returnThing;
            } else {
                return error.Else;
            }
        };
        if (result.output.len > 0) { //halted
            const thing = args5.hashmap.getOrPut(size) catch return error.Else;
            if (thing.found_existing) {
                thing.value_ptr.* += 1;
            } else {
                thing.value_ptr.* = 1;
            }
        }
        const thing = args5.totalsHashmap.getOrPut(size) catch return error.Else;
        if (thing.found_existing) {
            thing.value_ptr.* += 1;
        } else {
            thing.value_ptr.* = 1;
        }
    }

    // std.debug.print("\n{s} size {} =>, {s}", .{ input, result.inputSize, result.output });

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

        try streamBinaryStrsRange(a, b, &exp1_noteOutputs, arena);
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

        try streamBinaryStrsRange(a, b, &exp2_noteHaltedInputs, args2);
    }

    //experiment 3, collecting frequencies of program sizes
    // {
    //     var a = try BigIntManaged.init(arena);
    //     var b = try BigIntManaged.init(arena);

    //     defer b.deinit();
    //     defer a.deinit();

    //     try a.set(0);
    //     try b.set(2_000_000);

    //     var hashmap = std.AutoHashMap(u32, u32).init(arena);
    //     defer hashmap.deinit();

    //     try streamBinaryStrsRange(a, b, &experiment3, .{ .allocator = arena, .hashmap = &hashmap });

    //     var it = hashmap.iterator();
    //     std.debug.print("\n", .{});
    //     while (it.next()) |entry| {
    //         // if (entry.key_ptr.* > 100) continue;
    //         std.debug.print("{},{}\n", .{ entry.key_ptr.*, entry.value_ptr.* });
    //     }
    // }

    //experiment4 estimated max program sizes
    // {
    //     for (1..100) |i| {
    //         std.debug.print("\nsize {}, maxPrograms->{f}", .{ i, try T3.maxEstimatedProgramsOfSize(i) });
    //     }
    // }

    // TODO work on this one, calculate the frequencies, print out a csv for the plots
    // it has to be modified as well, the maxSteps should change per program size i think.
    //experiemnt 5
    {
        var hashmap = std.AutoHashMap(u32, u32).init(arena);
        defer hashmap.deinit();

        var totalsHashmap = std.AutoHashMap(u32, u32).init(arena);
        defer totalsHashmap.deinit();

        const args: exp5args = .{
            .allocator = arena, //
            .hashmap = &hashmap,
            .maxSteps = 100,
            .totalsHashmap = &totalsHashmap,
        };

        var a = try BigIntManaged.init(arena);
        var b = try BigIntManaged.init(arena);

        try a.set(0);
        try b.set(100);

        defer b.deinit();
        defer a.deinit();

        try streamBinaryStrsRange(a, b, &exp5, args);
    }

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
