const std = @import("std");
const LambdaToken = @import("LambdaLexer.zig").LambdaToken;
const LambdaParser = @import("LambdaParser.zig").LambdaParser;
const LambdaExpression = @import("LambdaParser.zig").LambdaExpression;

pub const LambdaVarNumType: type = u32;

pub const LambdaInterpreter = struct {
    allocator: std.mem.Allocator,
    replaceExpr: ?*LambdaExpression,
    traverseCounter: ?u32 = null,

    pub fn init(allocator: std.mem.Allocator) LambdaInterpreter{
        return .{
            .allocator=allocator,
            .replaceExpr=null
        };
    }

    fn decrementVarsTraverse(foreachExpr: *LambdaExpression, interp:anytype, lambdaHeight:u32) error{Yield}!void {
        if(interp.traverseCounter) |traverseCount|{
            if(traverseCount == 0){return error.Yield;}
            else {interp.traverseCounter = traverseCount - 1;}
        }
        switch(foreachExpr.*){
            .variable => |varExpr| {
                if(varExpr.value != lambdaHeight){
                    std.debug.print("decremented\n",.{});
                    foreachExpr.variable.value = foreachExpr.variable.value - 1;
                }
            },
            else => {}
        }
    }

    fn replaceVarsWithExprClone(foreachExpr: *LambdaExpression, interp:anytype, lambdaHeight:u32) error{Yield}!void {
        if(interp.traverseCounter) |traverseCount|{
            if(traverseCount == 0){return error.Yield;}
            else {interp.traverseCounter = traverseCount - 1;}
        }
        switch(foreachExpr.*){
            .variable => |varExpr| {
                if(varExpr.value == lambdaHeight){
                    if(interp.replaceExpr) |replaceExpr| {
                        // std.debug.print("\ncopied: ", .{});
                        // replaceExpr.traversePrint();
                        // std.debug.print("\n", .{});

                        foreachExpr.* = (replaceExpr.deepClone(interp.allocator) catch unreachable).*;
                    }
                }
                else if(varExpr.value > lambdaHeight){
                    // std.debug.print("decremented\n",.{});
                    foreachExpr.variable.value = foreachExpr.variable.value - 1;
                }
            },
            else => {}
        }
    }

    pub fn reduce(self:*LambdaInterpreter, expr: *const LambdaExpression, maxTraverses: ?u32)
    error{
        CANT_REDUCE,
        MAX_TRAVERSES,
        OutOfMemory
    }!*LambdaExpression
    {
        switch(expr.*){
            .apply => |applyExpr| {
                switch(applyExpr.lexpr.*){
                    .lambda =>  {
                        var lambdaExprCopy = try applyExpr.lexpr.deepClone(self.allocator);

                        self.replaceExpr = try applyExpr.rexpr.deepClone(self.allocator);

                        // if(self.replaceExpr) |hehe| hehe.traversePrint();

                        self.traverseCounter = maxTraverses;

                        // lambdaExprCopy.traverse(decrementVarsTraverse, self, 0) catch return error.MAX_TRAVERSES;
                        lambdaExprCopy.traverse(replaceVarsWithExprClone, self, 0) catch return error.MAX_TRAVERSES;

                        return lambdaExprCopy.lambda.expr;
                    },
                    else=>{}
                }
            },
            else => {}
        }

        return error.CANT_REDUCE;
    }

    pub fn reduceUntilCantReduce(self:*LambdaInterpreter, expr: *const LambdaExpression, maxTraverses: ?u32, maxReducesMaybe: ?u32)
    error{
        MAX_REDUCES,
        MAX_TRAVERSES,
        OutOfMemory
    }!*LambdaExpression
    {
        var expression = try expr.deepClone(self.allocator);

        if(maxReducesMaybe) |maxReduces| {
            var reduces:u32 = 0;
            while(true){
                if(reduces >= maxReduces)return error.MAX_REDUCES;

                const resultExpr = self.reduce(expression, maxTraverses);
                if(resultExpr) |reducedExpr| {
                    expression = reducedExpr;
                    reduces = reduces + 1;
                }else |err| switch(err) {
                    error.CANT_REDUCE => return expression,
                    error.MAX_TRAVERSES => return error.MAX_TRAVERSES,
                    error.OutOfMemory => return error.OutOfMemory
                }
            }
        }else{
            while(true){
                const resultExpr = self.reduce(expression, maxTraverses);
                if(resultExpr) |reducedExpr| {
                    expression = reducedExpr;
                }else |err| switch(err) {
                    error.CANT_REDUCE => return expression,
                    error.MAX_TRAVERSES => return error.MAX_TRAVERSES,
                    error.OutOfMemory => return error.OutOfMemory
                }
            }
        }

    }


};