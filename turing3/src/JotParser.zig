const std = @import("std");
const LambdaExpression = @import("LambdaParser.zig").LambdaExpression;

/// converts binary into lambda expressions according to Jot.
/// see https://cleare.st/code/iota-jot-zot#jot , https://en.wikipedia.org/wiki/Iota_and_Jot#Jot



pub const JotParser = struct {
    allocator: std.mem.Allocator,

    
};