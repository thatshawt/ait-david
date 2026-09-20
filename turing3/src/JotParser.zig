const std = @import("std");
const LambdaExpression = @import("LambdaParser.zig").LambdaExpression;
const LambdaParser = @import("LambdaParser.zig").LambdaParser;
const LambdaBuilder = @import("LambdaBuilder.zig").LambdaBuilder;

pub fn JotParserMake(allocator: std.mem.Allocator) JotParser {
    return .{ .builder = .{ .allocator = allocator } };
}

const doDebugPrintParse = false;

fn debugPrintParser(comptime fmt: []const u8, args: anytype) void {
    if (doDebugPrintParse) {
        std.debug.print(fmt, args);
    }
}

/// converts binary into lambda expressions according to Jot.
/// see https://cleare.st/code/iota-jot-zot#jot , https://en.wikipedia.org/wiki/Iota_and_Jot#Jot , https://esolangs.org/wiki/Jot , http://web.archive.org/web/20201112014512/http://www.nyu.edu/projects/barker/Iota/
/// used to convert binary -> expression, good for input
pub const JotParser = struct {
    // allocator: std.mem.Allocator,
    builder: LambdaBuilder,

    pub fn parse(self: *const JotParser, binaryStr: []const u8) !*LambdaExpression {
        debugPrintParser("called parse()\n", .{});
        if (binaryStr.len == 0) {
            debugPrintParser("len() == 0\n", .{});
            // []      ->   λx.x
            // /(1)
            const expr = try self.builder.expression(.{ .text = "/(1)" });
            return expr;
        }

        switch (binaryStr[binaryStr.len - 1]) {
            '0' => {
                debugPrintParser("0\n", .{});
                // [F0]    ->   [F](λx.λy.λz.(xz)(yz))(λx.λy.x)
                // (( [F] /(/(/( ((3 1) (2 1)) )))) /(/(2)))
                return try self.builder.apply( //
                    .{
                        .expr = try self.builder.apply( //
                            .{ .expr = try self.parse(binaryStr[0 .. binaryStr.len - 1]) }, //
                            .{ .text = "/(/(/(((3 1) (2 1)))))" }),
                    }, //
                    .{ .text = "/(/(2))" });
            },
            '1' => {
                debugPrintParser("1\n", .{});
                // [F1]    ->   λx.λy.[F](x y)
                // /(/( (F (2 1)) ))
                return try self.builder.lambda(.{
                    .expr = try self.builder.lambda(.{
                        .expr = try self.builder.apply( //
                            .{ .expr = try self.parse(binaryStr[0 .. binaryStr.len - 1]) }, //
                            .{ .text = "(2 1)" }), //
                    }),
                });
            },
            else => {
                debugPrintParser("else?\n", .{});
                return try self.builder.expression(.{ .text = "/(1)" });
            }, //nothing i guess
        }
    }
};
