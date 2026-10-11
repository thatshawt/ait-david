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

    if (interpreter.reduceUntilCantReduce(topExpr, 400, 100, true)) |reducedExpr| {
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

    const iMax = 5;
    var iInt: u32 = 0;
    var list: ArrayList(u8) = .empty;
    // list.init(allocator);
    defer list.deinit(allocator);

    const jotParser: JotParser = JotParserMake(allocator);

    var fmtBuffer: [32]u8 = undefined;
    while (iInt < iMax) : (iInt += 1) {
        // if(iInt % 2 == 0)continue;
        @memset(&fmtBuffer, 0);

        try i.set(iInt);

        const binarySlice = try std.fmt.bufPrint(&fmtBuffer, "{b}", .{iInt});
        // std.debug.print("{s}|{s}\n\n", .{fmtBuffer});

        std.debug.print("{}|\"{s}\" =generateExpression> ", .{ iInt, fmtBuffer });
        const thing = try generate(allocator, &i);
        // std.debug.print("   ",.{});
        thing.traversePrint();

        std.debug.print("\n\"{s}\" =jotParse> ", .{fmtBuffer});
        const jotExpr = try jotParser.parse(binarySlice);
        jotExpr.traversePrint();
        std.debug.print("\n", .{});

        if (interpreter.reduceUntilCantReduce(jotExpr, 99, 99, true)) |reducedExpr| {
            std.debug.print("\t=> ", .{});
            reducedExpr.traversePrint();
        } else |err| switch (err) {
            else => {
                std.debug.print("\t=> {}", .{err});
            },
        }

        std.debug.print("\n", .{});
        thing.traversePrint();
        std.debug.print(" =blc> ", .{});

        // blcPrintExpression(thing);
        // std.debug.print("\n", .{});
        const blcSliceString = try loadBlcExpressionToSlice(thing, allocator);
        // for (list.items) |item| {
        //     std.debug.print("{}", .{item});
        // }
        // list.clearRetainingCapacity();
        std.debug.print("\"{s}\"", .{blcSliceString});

        std.debug.print("\n\n", .{});
    }

    const builder: LambdaBuilder = .{ .allocator = allocator };
    (try builder.apply(.{ .expr = try builder.apply(.{ .text = "999999" }, .{ .text = "/(/(/(((3 1) (2 1)))))" }) }, .{ .text = "/(/(2))" })).traversePrint();
}

pub fn main(init: std.process.Init) !void {
    std.debug.print("All your {s} are belong to us.\n", .{"codebase"});

    // This is appropriate for anything that lives as long as the process.
    const arena: std.mem.Allocator = init.arena.allocator();

    try doStuff(arena);

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
