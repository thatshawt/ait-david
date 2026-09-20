const std = @import("std");

const LambdaExpression = @import("LambdaParser.zig").LambdaExpression;
const LambdaParser = @import("LambdaParser.zig").LambdaParser;
const LambdaVarNumType = @import("LambdaInterpreter.zig").LambdaVarNumType;

const ExprOrText = union(enum) {
    expr: *LambdaExpression,
    text: []const u8,

    pub fn toExpr(self: *const ExprOrText, allocator: std.mem.Allocator) !*LambdaExpression {
        switch (self.*) {
            .expr => |lambdaExpr| {
                return lambdaExpr;
            },
            .text => |text| {
                const expr = try LambdaParser.parse(allocator, text);
                return expr;
            },
        }
    }
};

const VarValueOrText = union(enum) {
    value: LambdaVarNumType,
    text: []const u8,

    pub fn toVarValue(self: *const VarValueOrText, allocator: std.mem.Allocator) error{NotVariableValue}!LambdaVarNumType {
        switch (self.*) {
            .value => |value| {
                return value;
            },
            .text => |text| {
                const expr = try LambdaParser.parse(allocator, text);
                switch (expr.*) {
                    .lambda => {
                        return error.NotVariableValue;
                    },
                    .apply => {
                        return error.NotVariableValue;
                    },
                    .variable => |varExpr| {
                        return varExpr.value;
                    },
                }
            },
        }
    }
};

const doDebugPrintBuilder = false;

fn debugPrintBuilder(comptime fmt: []const u8, args: anytype) void {
    if (doDebugPrintBuilder) std.debug.print("LambdaBuilder::" ++ fmt, args);
}

pub const LambdaBuilder = struct {
    allocator: std.mem.Allocator,

    pub fn apply(self: *const LambdaBuilder, lexpr: ExprOrText, rexpr: ExprOrText) !*LambdaExpression {
        debugPrintBuilder("apply()\n", .{});
        const expr = try self.allocator.create(LambdaExpression);
        expr.* = .{ .apply = .{ .lexpr = try lexpr.toExpr(self.allocator), .rexpr = try rexpr.toExpr(self.allocator) } };
        return expr;
    }

    pub fn lambda(self: *const LambdaBuilder, expr: ExprOrText) !*LambdaExpression {
        debugPrintBuilder("lambda()\n", .{});
        const returnExpr = try self.allocator.create(LambdaExpression);
        returnExpr.* = .{ .lambda = .{ .expr = try expr.toExpr(self.allocator) } };
        return returnExpr;
    }

    pub fn variable(self: *const LambdaBuilder, value: VarValueOrText) !*LambdaExpression {
        debugPrintBuilder("variable()\n", .{});
        const expr = try self.allocator.create(LambdaExpression);
        expr.* = .{ .variable = .{ .value = try value.toVarValue(self.allocator) } };
        return expr;
    }

    pub fn expression(self: *const LambdaBuilder, expr: ExprOrText) !*LambdaExpression {
        debugPrintBuilder("expression()\n", .{});
        const returnExpr = try expr.toExpr(self.allocator);
        return returnExpr;
    }
};
