const std = @import("std");
const LambdaExpression = @import("LambdaParser.zig").LambdaExpression;
const LambdaParser = @import("LambdaParser.zig").LambdaParser;

/// converts binary into lambda expressions according to Jot.
/// see https://cleare.st/code/iota-jot-zot#jot , https://en.wikipedia.org/wiki/Iota_and_Jot#Jot

pub const JotParser = struct {
    // allocator: std.mem.Allocator,

    pub fn parse(allocator: std.mem.Allocator, binaryStr: []const u8) !*LambdaExpression{
        if(binaryStr.len == 0){
            // []      ->   λx.x
            // /(1)
            var expr = try allocator.create(LambdaExpression);
            expr.* = .{
                .lambda = .{
                    .expr = try LambdaParser.parse(allocator, "/(1)")
                }
            };
            return expr;
        }

        switch(binaryStr[binaryStr.len-1]){
            '0' => {
                // [F0]    ->   [F](λx.λy.λz.(xz)(yz))(λx.λy.x)
                // (( [F] /(/(/( ((3 1) (2 1)) )))) //(2))
                var expr = try allocator.create(LambdaExpression);
                expr.* = .{
                    .lambda = .{
                        .expr = try LambdaParser.parse(allocator, "/(1)")
                    }
                };
                return expr;
            },
            '1' => { 
            // [F1]    ->   λx.λy.[F](xy)
            // 

            },
            else => {} //nothing i guess
        }
    }
};