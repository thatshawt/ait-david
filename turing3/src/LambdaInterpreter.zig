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

    incrStartHeight: ?u32 = null,

    pub fn init(allocator: std.mem.Allocator) LambdaInterpreter {
        return .{ .allocator = allocator, .replaceExpr = null };
    }

    fn decrementFreeVarsTraverse(foreachExpr: *LambdaExpression, interp: anytype, lambdaHeight: u32) error{Yield}!void {
        if (interp.reduceTraverseCounter) |traverseCount| {
            if (traverseCount == 0) {
                return error.Yield;
            } else {
                interp.reduceTraverseCounter = traverseCount - 1;
            }
        }
        switch (foreachExpr.*) {
            .variable => |varExpr| {
                if (varExpr.value > lambdaHeight) {
                    reductionDebugPrint("decremented\n", .{});
                    foreachExpr.variable.value = foreachExpr.variable.value - 1;
                }
            },
            else => {},
        }
    }

    fn incrementFreeVarsTraverse(foreachExpr: *LambdaExpression, interp: anytype, lambdaHeight: u32) error{Yield}!bool {
        if (interp.reduceTraverseCounter) |traverseCount| {
            if (traverseCount == 0) {
                return error.Yield;
            } else {
                interp.reduceTraverseCounter = traverseCount - 1;
            }
        }
        switch (foreachExpr.*) {
            .variable => |varExpr| {
                if (interp.incrStartHeight) |incrStartHeight| {
                    reductionDebugPrint("checking incrementFreeVar {} {} {}\n", .{ varExpr.value, lambdaHeight, incrStartHeight });
                    if (varExpr.value > (lambdaHeight)) {
                        // reductionDebugPrint("incremented by {}\n", .{incrStartHeight - lambdaHeight + 1});
                        // foreachExpr.variable.value = foreachExpr.variable.value + incrStartHeight - lambdaHeight + 1;
                        reductionDebugPrint("incremented by {}\n", .{incrStartHeight});
                        foreachExpr.variable.value = foreachExpr.variable.value + incrStartHeight;
                    }
                }
            },
            else => {},
        }
        return true;
    }

    // CHECK THIS AGAIN CUS I MIGHT HAVE FIXED IT WITH THE BOOL RETURN VALUE
    // TODO: when reducing we need to NOT modify the expressing we are traversing in...
    // probably send the modifications to another copy expression or smth...
    // the problem is that after modifying the expression, the traversal might visit the modified expression wich leads to endless looping if there is a variable that matches the height...
    fn replaceVarsWithExprClone(foreachExpr: *LambdaExpression, interp: anytype, lambdaHeight: u32) error{Yield}!bool {
        // const interp: LambdaInterpreter = @as(arg, LambdaInterpreter);
        if (interp.reduceTraverseCounter) |traverseCount| {
            if (traverseCount <= 0) {
                return error.Yield;
            } else {
                interp.reduceTraverseCounter = traverseCount - 1;
            }
        }

        if (doReductionDebugPrint) {
            std.debug.print("replacing: ", .{});
            foreachExpr.traversePrint();
            std.debug.print("\n", .{});
        }

        switch (foreachExpr.*) {
            .variable => |varExpr| {
                reductionDebugPrint("var {}, height {}\n", .{ varExpr.value, lambdaHeight });
                if (varExpr.value == lambdaHeight) {
                    if (interp.replaceExpr) |replaceExpr| {
                        if (doReductionDebugPrint) {
                            std.debug.print("copied: ", .{});
                            replaceExpr.traversePrint();
                            std.debug.print("\n", .{});
                        }

                        foreachExpr.* = (replaceExpr.deepClone(interp.allocator) catch unreachable).*;

                        interp.incrStartHeight = lambdaHeight - 1;
                        try foreachExpr.traverse(incrementFreeVarsTraverse, interp, 0);

                        return false; // do not continue traversing
                    }
                } else if (varExpr.value > lambdaHeight) {
                    // std.debug.print("decremented\n",.{});
                    foreachExpr.variable.value = foreachExpr.variable.value - 1;
                }
            },
            else => {},
        }
        return true; //continuie traversing
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

                        reductionDebugPrint("starting traverse with {?} traverses left\n", .{self.reduceTraverseCounter});

                        // lambdaExprCopy.traverse(decrementVarsTraverse, self, 0) catch return error.MAX_TRAVERSES;
                        lambdaExprCopy.traverse(replaceVarsWithExprClone, self, 0) catch {
                            reductionDebugPrint("({?} traverses left) running the traverse returned with MAX_TRAVERSES\n", .{self.reduceTraverseCounter});
                            return error.MAX_TRAVERSES;
                        };

                        return lambdaExprCopy.lambda.expr;
                    },
                    else => {},
                }
            },
            else => {},
        }

        return error.CANT_REDUCE;
    }

    fn greedyReduceTraversal(expr: *LambdaExpression, interp: anytype, lambdaHeight: u32) error{Yield}!bool {
        _ = lambdaHeight;
        if (interp.greedyTraverseCounter) |traverseCount| {
            if (traverseCount <= 0) {
                reductionDebugPrint("traverse count below zero\n", .{});
                return error.Yield;
            } else {
                interp.greedyTraverseCounter = traverseCount - 1;
            }
        }

        if (doReductionDebugPrint) {
            std.debug.print("({?} traverses left)trying reduce: ", .{interp.greedyTraverseCounter});
            expr.traversePrint();
            std.debug.print("\n", .{});
        }

        const reducedExprResult = interp.reduce(expr, interp.greedyTraverseCounter);

        if (reducedExprResult) |reducedExpr| {
            interp.greedyReduced = reducedExpr;
            expr.* = reducedExpr.*;

            return error.Yield;
        } else |err| switch (err) {
            error.OutOfMemory => {
                interp.greedyErrorReturn = error.OutOfMemory;
                reductionDebugPrint("ran out of memory\n", .{});
                return error.Yield;
            },
            error.MAX_TRAVERSES => {
                reductionDebugPrint("max traverses returned by interp.reduce\n", .{});
                return error.Yield;
            },
            else => {
                return true; // continue traversing
            },
        }
    }

    /// tries to reduce every sub-expression, halts when it reduces one or if it tries every expression.
    pub fn greedyTraverseReduce(self: *LambdaInterpreter, expr: *const LambdaExpression, maxTraverses: ?u32) error{ CANT_REDUCE, MAX_TRAVERSES, OutOfMemory }!*LambdaExpression {
        // run the traverse greedy reduce on top expression
        self.greedyTraverseCounter = maxTraverses;
        self.greedyReduced = null;
        self.greedyErrorReturn = null;

        const returnExpr = try expr.deepClone(self.allocator);

        if (doReductionDebugPrint) {
            reductionDebugPrint("greedy reducing: ", .{});
            returnExpr.traversePrint();
            std.debug.print("\n", .{});
        }

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
                    reductionDebugPrint("greedy traversal Yielded and hit ELSE block\n", .{});
                    return error.MAX_TRAVERSES;
                }
            },
        }
    }

    const doReductionDebugPrint = false;
    fn reductionDebugPrint(comptime fmt: []const u8, args: anytype) void {
        if (doReductionDebugPrint) std.debug.print("Reduction::" ++ fmt, args);
    }
    pub fn reduceUntilCantReduce(self: *LambdaInterpreter, expr: *const LambdaExpression, maxTraverses: ?u32, maxReducesMaybe: ?u32, greedy: bool) error{ MAX_REDUCES, MAX_TRAVERSES, OutOfMemory }!*LambdaExpression {
        var expression = try expr.deepClone(self.allocator);

        if (maxReducesMaybe) |maxReduces| {
            var reduces: u32 = 0;
            while (true) {
                if (reduces >= maxReduces) return error.MAX_REDUCES;

                const resultExpr =
                    if (greedy) self.greedyTraverseReduce(expression, maxTraverses) else self.reduce(expression, maxTraverses);

                if (resultExpr) |reducedExpr| {
                    if (doReductionDebugPrint) {
                        std.debug.print("reduced to: ", .{});
                        reducedExpr.traversePrint();
                        std.debug.print("\n", .{});
                    }
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
                    reductionDebugPrint("\n", .{});
                } else |err| switch (err) {
                    error.CANT_REDUCE => return expression,
                    error.MAX_TRAVERSES => return error.MAX_TRAVERSES,
                    error.OutOfMemory => return error.OutOfMemory,
                }
            }
        }
    }
};
