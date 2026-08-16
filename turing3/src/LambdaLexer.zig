const std = @import("std");
const LambdaVarNumType = @import("LambdaInterpreter.zig").LambdaVarNumType;

pub const LambdaTokenType = enum {
    LPAREN,
    RPAREN,
    LAMBDA,
    VAR_NUM,
    EOF
};

pub const LambdaToken = struct {
    tokenType: LambdaTokenType,
    value: LambdaVarNumType,

    pub fn print(self: *const LambdaToken) void {
        return switch(self.tokenType){
            LambdaTokenType.LPAREN => std.debug.print("(", .{}),
            LambdaTokenType.RPAREN => std.debug.print(")", .{}),
            LambdaTokenType.LAMBDA => std.debug.print("/", .{}),
            LambdaTokenType.VAR_NUM => std.debug.print("{}", .{self.value}),
            LambdaTokenType.EOF => std.debug.print("EOF", .{}),
        };
    }
};

pub const LambdaLexer = struct {
    buffer: []const u8,
    index: usize,
    done: bool,

    pub fn init(buffer: []const u8) LambdaLexer {
        return .{
            .buffer= buffer,
            .index= 0,
            .done=false
        };
    }

    fn peek(self: *LambdaLexer) ?u8 {
        if(self.index >= self.buffer.len){
            return null;
        }
        else{
            return self.buffer[self.index];
        }
    }

    fn advance(self: *LambdaLexer) ?u8 {
        const char = self.peek();
        if(char != null) self.index += 1;
        return char;
    }

    fn skipWhitespace(self: *LambdaLexer) void {
        while(self.peek()) |c| {
            if(c == ' ' or c == '\t' or c == '\n' or c == '\r'){
               _ = self.advance();
            }else{
                break;
            }

        }
    }

    pub fn nextToken(self: *LambdaLexer) ?LambdaToken {
        if(self.done)return null;

        self.skipWhitespace();

        const start = self.index;
        const char = self.advance();
        if(char == null){
            self.done = true;
            return .{.tokenType=LambdaTokenType.EOF, .value=0};
        }

        return switch(char orelse ' '){
            '/' => .{.tokenType=LambdaTokenType.LAMBDA, .value=0},
            '(' => .{.tokenType=LambdaTokenType.LPAREN, .value=0},
            ')' => .{.tokenType=LambdaTokenType.RPAREN, .value=0},
            '0'...'9' => {
                while(self.peek())|next|{
                    if(next >= '0' and next <= '9'){
                        _ = self.advance();
                    }else{
                        break;
                    }
                }

                const parseNum = std.fmt.parseInt(LambdaVarNumType, self.buffer[start..self.index], 10) catch 999;

                return .{.tokenType=LambdaTokenType.VAR_NUM, .value=parseNum};
            },
            else => self.nextToken() // ignore everything else 
        };
    }
};