const std = @import("std");
const BigIntManaged = std.math.big.int.Managed;


/// test if the pair does not return the same number for different pairs
// test cantorPair {

// }

/// r = (n+m)(n+m+1)/2 + m
pub fn cantorPair(
    allocator: std.mem.Allocator,
    r: *BigIntManaged,
    n: *const BigIntManaged,
    m: *const BigIntManaged 
    ) !void {

    var temp1 = try BigIntManaged.init(allocator);
    var temp2 = try BigIntManaged.init(allocator);
    defer temp1.deinit();
    defer temp2.deinit();

    try temp1.add(n,m);
    
    try temp2.copy(temp1.toConst());
    try r.set(1);
    try temp2.add(r, &temp2);
    
    try r.mul(&temp1, &temp2);

    try temp1.set(2);
    try r.divFloor(&temp2, r, &temp1);

    try r.add(r, m);
}

/// https://en.wikipedia.org/wiki/Pairing_function#Inverting_the_Cantor_pairing_function
pub fn cantorUnpair(
    allocator: std.mem.Allocator,
    r: *const BigIntManaged,
    n: *BigIntManaged,
    m: *BigIntManaged
    ) !void {
    
    var temp1 = try BigIntManaged.init(allocator);
    var temp2 = try BigIntManaged.init(allocator);
    var temp3 = try BigIntManaged.init(allocator);
    var temp4 = try BigIntManaged.init(allocator);
    var temp5 = try BigIntManaged.init(allocator);
    defer temp1.deinit();
    defer temp2.deinit();
    defer temp3.deinit();
    defer temp4.deinit();
    defer temp5.deinit();

    // w = floor( (sqrt(8*r + 1) - 1) / 2 )
    try temp1.set(8);
    try temp1.mul(&temp1, r);
    try temp2.set(1);
    try temp1.add(&temp1, &temp2);
    try temp1.sqrt(&temp1);

    try temp2.set(1);
    try temp1.sub(&temp1, &temp2);

    try temp2.set(2);
    try temp1.divFloor(&temp3, &temp1, &temp2);
    // temp1 = w
    
    // t = (w*w + w) / 2
    try temp2.sqr(&temp1);
    try temp2.add(&temp2, &temp1);
    try temp4.set(2);
    try temp2.divFloor(&temp3, &temp2, &temp4);
    // temp2 = t

    // m = r - t
    try m.sub(r, &temp2);

    // n = w - m
    try n.sub(&temp1, m);
}