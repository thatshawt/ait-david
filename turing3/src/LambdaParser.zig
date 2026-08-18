const std = @import("std");
const LambdaLexer = @import("LambdaLexer.zig").LambdaLexer;
const LambdaToken = @import("LambdaLexer.zig").LambdaToken;
const LambdaTokenType = @import("LambdaLexer.zig").LambdaTokenType;
const LambdaVarNumType = @import("LambdaInterpreter.zig").LambdaVarNumType;

// var = positive integer or zero
// lambda = '/'
// expr = lambda '(' expr ')' | var | '(' expr expr ')'

pub const LambdaExpression = union(enum) {
    lambda: struct{
        expr: *LambdaExpression
    },
    variable: struct{
        value: LambdaVarNumType
    },
    apply: struct{
        lexpr: *LambdaExpression,
        rexpr: *LambdaExpression
    },

    pub fn deepClone(self: *const LambdaExpression, allocator: std.mem.Allocator) !*LambdaExpression{
        return switch(self.*){
            .lambda => |lambdaExpr| {
                const exprPtr = try allocator.create(LambdaExpression);
                exprPtr.* = .{
                    .lambda=.{
                        .expr=try lambdaExpr.expr.deepClone(allocator)
                    }
                };
                return exprPtr;
            },
            .variable => |varExpr| {
                const exprPtr = try allocator.create(LambdaExpression);
                exprPtr.* = .{
                    .variable=.{
                        .value=varExpr.value
                    }
                };
                return exprPtr;
            },
            .apply => |applyExpr| {
                const exprPtr = try allocator.create(LambdaExpression);
                exprPtr.* = .{
                    .apply=.{
                        .lexpr=try applyExpr.lexpr.deepClone(allocator),
                        .rexpr=try applyExpr.rexpr.deepClone(allocator)
                    }
                };
                return exprPtr;
            }
        };

    }

    pub fn traverse(self: *LambdaExpression, comptime foreach: *const fn(*LambdaExpression, anytype, u32) error{Yield}!void, args:anytype, lambdaHeight:u32)
    error{Yield}!void {
        try foreach(self, args, lambdaHeight);
        switch(self.*){
            .lambda => |lambdaExpr| {
                try lambdaExpr.expr.traverse(foreach, args, lambdaHeight+1);
            },
            .variable => {
                //nothing i guess
            },
            .apply => |applyExpr| {
                try applyExpr.lexpr.traverse(foreach, args, lambdaHeight);
                try applyExpr.rexpr.traverse(foreach, args, lambdaHeight);
            }
        }
    }

    pub fn traversePrint(self: *const LambdaExpression) void {
        switch(self.*){
            .lambda => |lambdaExpr| {
                std.debug.print("/",.{});
                LambdaExpression.traversePrint(lambdaExpr.expr);
                std.debug.print("",.{});
            },
            .variable => |varExpr| {
                std.debug.print("{}",.{varExpr.value});
            },
            .apply => |applyExpr| {
                std.debug.print("(", .{});
                LambdaExpression.traversePrint(applyExpr.lexpr);
                std.debug.print(" ", .{});
                LambdaExpression.traversePrint(applyExpr.rexpr);
                std.debug.print(")", .{});
            }
        }
    }

    /// start this one token after the open(left) paren.
    /// returns the index one after the matching close(right) paren.
    fn findMatchCloseParens(tokenSlice: []const LambdaToken) error{DID_NOT_FIND_MATCHING_PAREN}!usize {
        var paren: u32 = 1;
        for (tokenSlice, 0..) |token, index| {
            if(paren == 0){
                std.debug.print("\n",.{});
                return index;
            }
            // std.debug.print("{}: ",.{index});
            token.print();
            std.debug.print(", ",.{});
            paren = switch(token.tokenType){
                LambdaTokenType.LPAREN  => paren+1,
                LambdaTokenType.RPAREN  => paren-1,
                else  => paren
            };
        }
        std.debug.print("\n",.{});
        if(paren == 0)return tokenSlice.len;
        return error.DID_NOT_FIND_MATCHING_PAREN;
    }

    fn printNTimesTabs(N:u32) void {
        var i:u32 = 0;
        while (i < N) : (i += 1) {
            std.debug.print("\t", .{});
        }
    }

    pub fn parseFromTokens(allocator: std.mem.Allocator, tokenSlice: []const LambdaToken, height: u32) 
    error{
        NO_PAREN_AFTER_LAMBDA,
        UNBALANCED_RPAREN_IDK,
        REACHED_EOF_IDK,
        DID_NOT_FIND_MATCHING_PAREN,
        OutOfMemory
    }!struct{
        expression:*LambdaExpression,
        endIndex: usize
    }{
        printNTimesTabs(height);
        std.debug.print("current token slice: ", .{});
        LambdaToken.printTokenSlice(tokenSlice);
        // std.debug.print("tokens: {}, current token: ",.{tokenSlice.len});
        // tokenSlice[0].print();
        // std.debug.print(", last token: ",.{});
        // tokenSlice[tokenSlice.len-1].print();
        std.debug.print("\n",.{});
        return switch(tokenSlice[0].tokenType){
            LambdaTokenType.LAMBDA  => {
                printNTimesTabs(height);
                std.debug.print("parsing lambda\n",.{});
                if(tokenSlice[1].tokenType != LambdaTokenType.LPAREN){
                    tokenSlice[1].print();
                    return error.NO_PAREN_AFTER_LAMBDA;
                }
                const matchingParenIndex = try LambdaExpression.findMatchCloseParens(tokenSlice[2..]);

                printNTimesTabs(height);
                std.debug.print("\nmatchingparenIndex:{d}\n",.{matchingParenIndex});

                const parsedExprPtr = try LambdaExpression.parseFromTokens(allocator, tokenSlice[2..matchingParenIndex+1], height+1);

                const exprPtr = try allocator.create(LambdaExpression);
                exprPtr.* = .{
                    .lambda=.{
                        .expr=parsedExprPtr.expression
                    }
                };

                printNTimesTabs(height);
                std.debug.print("added lambda\n", .{});

                return .{.expression=exprPtr, .endIndex=matchingParenIndex+2};
            },
            LambdaTokenType.LPAREN  => {
                printNTimesTabs(height);
                std.debug.print("parsing lparen\n",.{});
                const matchingParenIndex = try LambdaExpression.findMatchCloseParens(tokenSlice[1..]);

                printNTimesTabs(height);
                std.debug.print("parsing lexpr\n",.{});
                const lParsedExpr = try LambdaExpression.parseFromTokens(allocator,
                    tokenSlice[1 .. matchingParenIndex] ,height+1);

                printNTimesTabs(height);
                std.debug.print("lexpr endindex {}\n", .{lParsedExpr.endIndex});

                printNTimesTabs(height);
                std.debug.print("parsing rexpr\n",.{});

                const rParsedExpr = switch(lParsedExpr.expression.*){
                    .apply => try LambdaExpression.parseFromTokens(allocator,
                        tokenSlice[(lParsedExpr.endIndex+2) .. matchingParenIndex] ,height+1),
                    else => try LambdaExpression.parseFromTokens(allocator,
                        tokenSlice[(lParsedExpr.endIndex+1) .. matchingParenIndex] ,height+1)
                };

                

                const exprPtr = try allocator.create(LambdaExpression);
                exprPtr.* = .{
                    .apply=.{
                        .lexpr=lParsedExpr.expression,
                        .rexpr=rParsedExpr.expression
                    }
                };

                printNTimesTabs(height);
                std.debug.print("added apply\n", .{});

                printNTimesTabs(height);
                std.debug.print("returned endindex {}\n",.{matchingParenIndex});
                return .{.expression=exprPtr, .endIndex=matchingParenIndex};
            },
            LambdaTokenType.RPAREN  => error.UNBALANCED_RPAREN_IDK,
            LambdaTokenType.VAR_NUM => {
                printNTimesTabs(height);
                std.debug.print("parsing var\n",.{});
                const exprPtr = try allocator.create(LambdaExpression);

                exprPtr .* = .{
                    .variable=.{
                        .value=tokenSlice[0].value
                    }
                };

                printNTimesTabs(height);
                std.debug.print("added var\n", .{});

                printNTimesTabs(height);
                std.debug.print("returned endindex {}\n",.{1});

                return .{.expression=exprPtr, .endIndex=1};
            },
            LambdaTokenType.EOF     => error.REACHED_EOF_IDK
        };
    }
};

pub const LambdaParser = struct {

    pub fn parse(allocator: std.mem.Allocator, text: []const u8) !*const LambdaExpression {
        // convert program into tokens
        var lexer = LambdaLexer.init(text);

        var tokens: std.ArrayList(LambdaToken) = .empty;
        defer tokens.deinit(allocator);

        while(lexer.nextToken()) |token| {
            try tokens.append(allocator, token);
            // token.print();
            // std.debug.print(" ",.{});
        }

        // parse top expression
        const parsed = try LambdaExpression.parseFromTokens(allocator, tokens.items, 0);

        // parsed.expression.traversePrint();
        // std.debug.print("\n",.{});

        return parsed.expression;
    }

    // pub fn deinit(self: *LambdaParser) void{
    //     self.tokens.deinit(self.allocator);
    // }
};