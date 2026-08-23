const std = @import("std");

const LambdaExpression = @import("LambdaParser.zig").LambdaExpression;
const LambdaVarNumType = @import("LambdaInterpreter.zig").LambdaVarNumType;

const BigIntManaged = std.math.big.int.Managed;
const cantorPair = @import("PairingFunction.zig").cantorPair;
const cantorUnpair = @import("PairingFunction.zig").cantorUnpair;

// https://cs.stackexchange.com/questions/97539/lambda-calculus-generator
pub fn generate(allocator: std.mem.Allocator, i: *const BigIntManaged) !*LambdaExpression {
    var temp1 = try BigIntManaged.init(allocator);
    var j = try BigIntManaged.init(allocator);
    var k = try BigIntManaged.init(allocator);
    var temp3 = try BigIntManaged.init(allocator);
    defer temp1.deinit();
    defer j.deinit();
    defer k.deinit();
    defer temp3.deinit();

    if(i.isEven()){ // if i is even then return var(j=i/2);
        try j.set(2);
        try temp1.copy(i.toConst());
        try temp1.divFloor(&temp3, &temp1, &j);

        //print
        // const varStr = try temp1.toString(allocator, 10, std.fmt.Case.lower);
        // defer allocator.free(varStr);
        // std.debug.print("x{s} ", .{varStr}); // var(j)

        const varExpr = try allocator.create(LambdaExpression);
        varExpr.* = .{
            .variable = .{
                .value = (try temp1.toInt(LambdaVarNumType)) + 1
            }
        };
        return varExpr;

    }else{ // i is odd => j = (i-1)/2
        try j.copy(i.toConst());
        try temp1.set(1);
        try j.sub(&j, &temp1);
        try temp1.set(2);
        try j.divFloor(&temp3, &j, &temp1);

        if(j.isEven()){ // if even, k=j/2, find n,m such that pair(n,m)=k, N=generate(n), M=generate(m), return application(N M)
            try temp3.set(2);
            try k.divFloor(&temp1, &j, &temp3);

            try cantorUnpair(allocator, &k, &temp1, &temp3);

            //print
            // std.debug.print("(", .{}); // (

            const applyExpr = try allocator.create(LambdaExpression);

            applyExpr.* = .{
                .apply = .{
                    .lexpr = try generate(allocator, &temp1),
                    .rexpr = try generate(allocator, &temp3)
                }
            };
            // try generate(allocator, &temp1); // N
            // try generate(allocator, &temp3); // M
            // std.debug.print(")", .{}); // )

            return applyExpr;

        }else{ // if odd, k=(j-1)/2, find n,m such that pair(n,m)=k, M=generate(m), return lambda(var(n) . M)
            try temp3.set(1);
            try k.sub(&j, &temp3);
            try temp1.set(2);
            try k.divFloor(&temp3, &k, &temp1);

            try cantorUnpair(allocator, &k, &temp1, &temp3);

            //print
            // const varStr = try temp1.toString(allocator, 10, std.fmt.Case.lower);
            // defer allocator.free(varStr);
            // std.debug.print("(/", .{}); // lambda(var(n) NAH

            //TODO do we need var(n) here? i didnt use it below...

            const lambdaExpr = try allocator.create(LambdaExpression);

            lambdaExpr.* = .{
                .lambda = .{
                    // .expr = try generate(allocator, &temp3)
                    .expr = try generate(allocator, &k)
                }
            };
            // try generate(allocator, &temp3); // M
            // std.debug.print(")", .{});// )

            return lambdaExpr;
        }
    }
}