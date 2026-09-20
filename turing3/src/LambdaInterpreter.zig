const std = @import("std");
const LambdaToken = @import("LambdaLexer.zig").LambdaToken;
const LambdaParser = @import("LambdaParser.zig").LambdaParser;
const LambdaExpression = @import("LambdaParser.zig").LambdaExpression;

pub const LambdaVarNumType: type = u32;

pub const LambdaInterpreter = struct {
    allocator: std.mem.Allocator,

    replaceExpr: ?*LambdaExpression,
    reduceTraverseCounter: ?u32 = null,

    greedyTraverseCounter: ?u32 = null,
    greedyErrorReturn: ?error{OutOfMemory} = null,
    greedyReduced: ?*LambdaExpression = null,

    pub fn init(allocator: std.mem.Allocator) LambdaInterpreter {
        return .{ .allocator = allocator, .replaceExpr = null };
    }

    fn decrementVarsTraverse(foreachExpr: *LambdaExpression, interp: anytype, lambdaHeight: u32) error{Yield}!void {
        if (interp.reduceTraverseCounter) |traverseCount| {
            if (traverseCount == 0) {
                return error.Yield;
            } else {
                interp.reduceTraverseCounter = traverseCount - 1;
            }
        }
        switch (foreachExpr.*) {
            .variable => |varExpr| {
                if (varExpr.value != lambdaHeight) {
                    std.debug.print("decremented\n", .{});
                    foreachExpr.variable.value = foreachExpr.variable.value - 1;
                }
            },
            else => {},
        }
    }

    fn replaceVarsWithExprClone(foreachExpr: *LambdaExpression, interp: anytype, lambdaHeight: u32) error{Yield}!void {
        if (interp.reduceTraverseCounter) |traverseCount| {
            if (traverseCount == 0) {
                return error.Yield;
            } else {
                interp.reduceTraverseCounter = traverseCount - 1;
            }
        }
        switch (foreachExpr.*) {
            .variable => |varExpr| {
                if (varExpr.value == lambdaHeight) {
                    if (interp.replaceExpr) |replaceExpr| {
                        // std.debug.print("\ncopied: ", .{});
                        // replaceExpr.traversePrint();
                        // std.debug.print("\n", .{});

                        foreachExpr.* = (replaceExpr.deepClone(interp.allocator) catch unreachable).*;
                    }
                }
                // else if(varExpr.value > lambdaHeight){
                //     // std.debug.print("decremented\n",.{});
                //     foreachExpr.variable.value = foreachExpr.variable.value - 1;
                // }
            },
            else => {},
        }
    }

    pub fn reduce(self: *LambdaInterpreter, expr: *const LambdaExpression, maxTraverses: ?u32) error{ CANT_REDUCE, MAX_TRAVERSES, OutOfMemory }!*LambdaExpression {
        switch (expr.*) {
            .apply => |applyExpr| {
                switch (applyExpr.lexpr.*) {
                    .lambda => {
                        var lambdaExprCopy = try applyExpr.lexpr.deepClone(self.allocator);

                        self.replaceExpr = try applyExpr.rexpr.deepClone(self.allocator);

                        // if(self.replaceExpr) |hehe| hehe.traversePrint();

                        self.reduceTraverseCounter = maxTraverses;

                        // lambdaExprCopy.traverse(decrementVarsTraverse, self, 0) catch return error.MAX_TRAVERSES;
                        lambdaExprCopy.traverse(replaceVarsWithExprClone, self, 0) catch return error.MAX_TRAVERSES;

                        return lambdaExprCopy.lambda.expr;
                    },
                    else => {},
                }
            },
            else => {},
        }

        return error.CANT_REDUCE;
    }

    fn greedyReduceTraversal(expr: *LambdaExpression, interp: anytype, lambdaHeight: u32) error{Yield}!void {
        _ = lambdaHeight;
        if (interp.greedyTraverseCounter) |traverseCount| {
            if (traverseCount == 0) {
                return error.Yield;
            } else {
                interp.greedyTraverseCounter = traverseCount - 1;
            }
        }

        const reducedExprResult = interp.reduce(expr, interp.greedyTraverseCounter);

        if (reducedExprResult) |reducedExpr| {
            interp.greedyReduced = reducedExpr;
            expr.* = reducedExpr.*;

            return error.Yield;
        } else |err| switch (err) {
            error.OutOfMemory => {
                interp.greedyErrorReturn = error.OutOfMemory;
                return error.Yield;
            },
            error.MAX_TRAVERSES => return error.Yield,
            else => {},
        }
    }

    /// tries to reduce every sub-expression, halts when it reduces one or if it tries every expression.
    pub fn greedyTraverseReduce(self: *LambdaInterpreter, expr: *const LambdaExpression, maxTraverses: ?u32) error{ CANT_REDUCE, MAX_TRAVERSES, OutOfMemory }!*LambdaExpression {
        // run the traverse greedy reduce on top expression
        self.greedyTraverseCounter = maxTraverses;
        self.greedyReduced = null;
        self.greedyErrorReturn = null;

        const returnExpr = try expr.deepClone(self.allocator);

        const traverseResult = returnExpr.traverse(greedyReduceTraversal, self, 0);

        if (traverseResult) |traverseResultVoid| {
            _ = traverseResultVoid;

            if (self.greedyErrorReturn) |err|
                if (err == error.OutOfMemory) return error.OutOfMemory;

            if (self.greedyReduced) |reducedExpr| {
                _ = reducedExpr;
                return returnExpr;
            } else {
                return error.CANT_REDUCE;
            }
        } else |err| switch (err) {
            error.Yield => {
                if (self.greedyReduced) |reducedExpr| {
                    _ = reducedExpr;
                    return returnExpr;
                } else if (self.greedyErrorReturn) |returnedErr| {
                    return returnedErr;
                } else {
                    return error.MAX_TRAVERSES;
                }
            },
        }
    }

    const doReductionDebugPrint = false;

    pub fn reduceUntilCantReduce(self: *LambdaInterpreter, expr: *const LambdaExpression, maxTraverses: ?u32, maxReducesMaybe: ?u32, greedy: bool) error{ MAX_REDUCES, MAX_TRAVERSES, OutOfMemory }!*LambdaExpression {
        var expression = try expr.deepClone(self.allocator);

        if (maxReducesMaybe) |maxReduces| {
            var reduces: u32 = 0;
            while (true) {
                if (reduces >= maxReduces) return error.MAX_REDUCES;

                const resultExpr =
                    if (greedy) self.greedyTraverseReduce(expression, maxTraverses) else self.reduce(expression, maxTraverses);

                if (resultExpr) |reducedExpr| {
                    if (doReductionDebugPrint) reducedExpr.traversePrint();
                    if (doReductionDebugPrint) std.debug.print("\n", .{});
                    expression = reducedExpr;
                    reduces = reduces + 1;
                } else |err| switch (err) {
                    error.CANT_REDUCE => return expression,
                    error.MAX_TRAVERSES => return error.MAX_TRAVERSES,
                    error.OutOfMemory => return error.OutOfMemory,
                }
            }
        } else {
            while (true) {
                const resultExpr = self.reduce(expression, maxTraverses);
                if (resultExpr) |reducedExpr| {
                    expression = reducedExpr;
                    if (doReductionDebugPrint) reducedExpr.traversePrint();
                    if (doReductionDebugPrint) std.debug.print("\n", .{});
                } else |err| switch (err) {
                    error.CANT_REDUCE => return expression,
                    error.MAX_TRAVERSES => return error.MAX_TRAVERSES,
                    error.OutOfMemory => return error.OutOfMemory,
                }
            }
        }
    }
};
