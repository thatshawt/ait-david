const std = @import("std");

const LambdaExpression = @import("LambdaParser.zig").LambdaExpression;

const ArrayList = std.ArrayList;

/// https://esolangs.org/wiki/Binary_lambda_calculus , https://tromp.github.io/cl/Binary_lambda_calculus.html
/// converts expression into binary according to Jon Tromp's Binary Lambda Calculus(BLC)
/// used to convert expression -> binary, good for output
pub fn blcPrintExpression(expr: *const LambdaExpression) void {
    switch (expr.*) {
        .lambda => |lambdaExpr| {
            std.debug.print("00 ", .{});
            blcPrintExpression(lambdaExpr.expr);
        },
        .variable => |varExpr| {
            for (0..varExpr.value) |_| {
                std.debug.print("1", .{});
            }
            std.debug.print("0 ", .{});
        },
        .apply => |applyExpr| {
            std.debug.print("01 ", .{});
            blcPrintExpression(applyExpr.lexpr);
            blcPrintExpression(applyExpr.rexpr);
        },
    }
}

fn loadBlcExpressionToListRecurse(
    expr: *const LambdaExpression, //
    allocator: std.mem.Allocator, //
    list: *ArrayList(u8),
) !void {
    switch (expr.*) {
        .lambda => |lambdaExpr| {
            // std.debug.print("00 ", .{});
            try list.appendSlice(allocator, "00");
            try loadBlcExpressionToListRecurse(lambdaExpr.expr, allocator, list);
        },
        .variable => |varExpr| {
            // for (0..varExpr.value) |_| {
            //     std.debug.print("1", .{});
            // }
            try list.appendNTimes(allocator, '1', varExpr.value);
            // std.debug.print("0 ", .{});
            try list.append(allocator, '0');
        },
        .apply => |applyExpr| {
            // std.debug.print("01 ", .{});
            try list.appendSlice(allocator, "01");
            try loadBlcExpressionToListRecurse(applyExpr.lexpr, allocator, list);
            try loadBlcExpressionToListRecurse(applyExpr.rexpr, allocator, list);
        },
    }
}

pub fn loadBlcExpressionToSlice(
    expr: *const LambdaExpression, //
    allocator: std.mem.Allocator,
) ![]const u8 {
    var list: ArrayList(u8) = .empty;
    try loadBlcExpressionToListRecurse(expr, allocator, &list);
    defer list.deinit(allocator); // not necessary cus we call toOwnedSlice

    return list.toOwnedSlice(allocator);
}
