const std = @import("std");

const LambdaExpression = @import("LambdaParser.zig").LambdaExpression;

pub fn expressionToBlc (expr: *const LambdaExpression) void {
    switch(expr.*){
        .lambda => |lambdaExpr| {
            std.debug.print("00 ",.{});
            expressionToBlc(lambdaExpr.expr);
        },
        .variable => |varExpr| {
            for(0..varExpr.value) |_| {
                std.debug.print("1", .{});
            }
            std.debug.print("0 ", .{});
        },
        .apply => |applyExpr| {
            std.debug.print("01 ", .{});
            expressionToBlc(applyExpr.lexpr);
            expressionToBlc(applyExpr.rexpr);
        }
    }
}