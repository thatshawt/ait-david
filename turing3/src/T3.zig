const std = @import("std");

const LambdaParser = @import("LambdaParser.zig").LambdaParser;
const LambdaInterpreter = @import("LambdaInterpreter.zig").LambdaInterpreter;

const LambdaBuilder = @import("LambdaBuilder.zig").LambdaBuilder;

const JotParser = @import("JotParser.zig").JotParser;
const JotParserMake = @import("JotParser.zig").JotParserMake;
const blcPrintExpression = @import("Blc.zig").blcPrintExpression;
const loadBlcExpressionToSlice = @import("Blc.zig").loadBlcExpressionToSlice;

const BigFloat = @import("BFP").BigFloat(.{ .Significand = f64, .Exponent = i64 });
const BigIntManaged = std.math.big.int.Managed;

const debugPrintON = false;
fn debugPrint(comptime fmt: []const u8, args: anytype) void {
    if (debugPrintON) std.debug.print(fmt, args);
}

pub const SizeAndOutput = struct { inputSize: u32, output: []const u8 };

pub const SizeTypeDivTen = enum {
    MINUS_FIVE, //
    MINUS_NINE,
    MINUS_THIRTEEN,
    MINUS_SEVENTEEN,
    MINUS_TWENTYONE,
};

pub fn sizeGetTypeDivTen(size: u32) ?SizeTypeDivTen {
    if (size >= 5 and (size - 5) % 10 == 0) return SizeTypeDivTen.MINUS_FIVE;
    if (size >= 9 and (size - 9) % 10 == 0) return SizeTypeDivTen.MINUS_NINE;
    if (size >= 13 and (size - 13) % 10 == 0) return SizeTypeDivTen.MINUS_THIRTEEN;
    if (size >= 17 and (size - 17) % 10 == 0) return SizeTypeDivTen.MINUS_SEVENTEEN;
    if (size >= 21 and (size - 21) % 10 == 0) return SizeTypeDivTen.MINUS_TWENTYONE;
    return null;
}

/// L is the last considered maxTraverses from the sampling phase.
/// b is the fitted parameter for the exponent in the exponential model in the fitting phase.
/// M is the maxTraverses value at which we capture 99.99999997% of halting machines (a good estimate)
pub fn getStepsCapture5Sigma(b: f64, L: f64) BigFloat {
    const e: BigFloat = .init(std.math.e);
    const twoSevenNum: BigFloat = .init(2.71828);
    const bTimesL: BigFloat = .init(b * L);
    const lottaSix: BigFloat = .init(6666666113);
    const oneFive: BigFloat = .init(1.5e-10);
    const bigB: BigFloat = .init(b);

    // M = -( ln( 1.5E-10 * (2.71828 ^(b*L) + 6666666113)) - b*L ) / (b)
    // M = (1.5E-10.mul(2.71828.pow(b*L).add(6666666113))).sub(b*L)).log2().div(e.log2()).div(b).neg()

    const M: BigFloat = oneFive.mul(twoSevenNum.pow(bTimesL).add(lottaSix)).log2().div(e.log2()).sub(bTimesL).div(bigB).neg();

    std.debug.print("\n {} exponent, {} steps -> {f} steps to capture\n", .{ b, L, M });

    return M;
}

//TODO revisit this to make it more accurate or something
// a=0.241582 b=1.01114 c=9.28113, maxProgs = a*b^(c*size)
// maxProgsEstimate = 0.241582 * 1.01114 ^ (9.28113 * size)
pub fn maxEstimatedProgramsOfSize(size: usize) !BigFloat {
    const oneThing = BigFloat.init(1.01114);
    const nineThing = BigFloat.init(9.28113);
    const sizeThing = BigFloat.init(size);
    const pointTwo = BigFloat.init(0.241582);

    return oneThing.pow(nineThing.mul(sizeThing)).mul(pointTwo);
}

fn firstIndexSeenFuncThing(r: *BigIntManaged, x: *const BigIntManaged, a: *const BigIntManaged, b: *const BigIntManaged, c: *const BigIntManaged, allocator: std.mem.Allocator) !void { //
    var temp1 = try BigIntManaged.init(allocator);
    var temp2 = try BigIntManaged.init(allocator);
    var temp3 = try BigIntManaged.init(allocator);
    defer temp1.deinit();
    defer temp2.deinit();
    defer temp3.deinit();

    try temp2.set(10);
    try temp1.sub(x, a);
    try temp1.divFloor(&temp3, &temp1, &temp2);
    try temp1.add(&temp1, b);

    try temp2.set(2);
    try temp1.pow(&temp2, try temp1.toInt(u32));

    try r.add(&temp1, c);
}

pub fn firstIndexSeenForProgramSize(r: *BigIntManaged, size: *const BigIntManaged, allocator: std.mem.Allocator) !bool { //
    var temp1 = try BigIntManaged.init(allocator);
    var temp2 = try BigIntManaged.init(allocator);
    var temp3 = try BigIntManaged.init(allocator);
    var temp4 = try BigIntManaged.init(allocator);
    defer temp1.deinit();
    defer temp2.deinit();
    defer temp3.deinit();
    defer temp4.deinit();

    //TODO remove needless operations
    try temp2.set(5);
    try temp1.sub(size, &temp2);
    try temp4.set(10);
    try temp1.divFloor(&temp3, &temp1, &temp4);
    const sizeInt = try size.toInt(u32);
    if (sizeInt >= 5 and (sizeInt - 5) % 10 == 0) {
        try temp1.set(0);
        try temp3.set(0);
        try firstIndexSeenFuncThing(r, size, &temp2, &temp1, &temp3, allocator);
        return true;
    }

    try temp2.set(9);
    try temp1.sub(size, &temp2);
    try temp4.set(10);
    try temp1.divFloor(&temp3, &temp1, &temp4);
    if (sizeInt >= 9 and (sizeInt - 9) % 10 == 0) {
        try temp1.set(0);
        try temp3.set(1);
        try firstIndexSeenFuncThing(r, size, &temp2, &temp1, &temp3, allocator);
        return true;
    }

    try temp2.set(13);
    try temp1.sub(size, &temp2);
    try temp4.set(10);
    try temp1.divFloor(&temp3, &temp1, &temp4);
    if (sizeInt >= 13 and (sizeInt - 13) % 10 == 0) {
        try temp1.set(2);
        try temp3.set(3);
        try firstIndexSeenFuncThing(r, size, &temp2, &temp1, &temp3, allocator);
        return true;
    }

    try temp2.set(17);
    try temp1.sub(size, &temp2);
    try temp4.set(10);
    try temp1.divFloor(&temp3, &temp1, &temp4);
    if (sizeInt >= 17 and (sizeInt - 17) % 10 == 0) {
        try temp1.set(3);
        try temp3.set(7);
        try firstIndexSeenFuncThing(r, size, &temp2, &temp1, &temp3, allocator);
        return true;
    }

    try temp2.set(21);
    try temp1.sub(size, &temp2);
    try temp4.set(10);
    try temp1.divFloor(&temp3, &temp1, &temp4);
    if (sizeInt >= 21 and (sizeInt - 21) % 10 == 0) {
        try temp1.set(4);
        try temp3.set(15);
        try firstIndexSeenFuncThing(r, size, &temp2, &temp1, &temp3, allocator);
        return true;
    }
    return false;
}

fn lastIndexSeenFuncThing(r: *BigIntManaged, x: *const BigIntManaged, a: *const BigIntManaged, b: *const BigIntManaged, c: *const BigIntManaged, allocator: std.mem.Allocator) !void { //
    var temp1 = try BigIntManaged.init(allocator);
    var temp2 = try BigIntManaged.init(allocator);
    var temp3 = try BigIntManaged.init(allocator);
    defer temp1.deinit();
    defer temp2.deinit();
    defer temp3.deinit();

    try temp2.set(4);
    try temp1.sub(x, a);
    try temp1.divFloor(&temp3, &temp1, &temp2);
    try temp1.add(&temp1, b);

    try temp2.set(2);
    try temp3.pow(&temp2, try temp1.toInt(u32));

    try r.sub(&temp3, c);
}

pub fn lastIndexSeenForProgramSize(r: *BigIntManaged, size: *const BigIntManaged, allocator: std.mem.Allocator) !bool { //
    var temp1 = try BigIntManaged.init(allocator);
    var temp2 = try BigIntManaged.init(allocator);
    var temp3 = try BigIntManaged.init(allocator);
    var temp4 = try BigIntManaged.init(allocator);
    defer temp1.deinit();
    defer temp2.deinit();
    defer temp3.deinit();
    defer temp4.deinit();

    const sizeInt = try size.toInt(i32);

    //TODO remove needless operations
    try temp2.set(17);
    try temp1.sub(size, &temp2);
    try temp4.set(4);
    try temp1.divFloor(&temp3, &temp1, &temp4);
    if (sizeInt >= 5 and @abs(sizeInt - 17) % 4 == 0) {
        try temp1.set(4);
        try temp3.set(1);
        try lastIndexSeenFuncThing(r, size, &temp2, &temp1, &temp3, allocator);
        return true;
    }

    try temp2.set(23);
    try temp1.sub(size, &temp2);
    try temp4.set(4);
    try temp1.divFloor(&temp3, &temp1, &temp4);
    if (sizeInt >= 9 and @abs(sizeInt - 23) % 4 == 0) {
        try temp1.set(4);
        try temp3.set(2);
        try lastIndexSeenFuncThing(r, size, &temp2, &temp1, &temp3, allocator);
        return true;
    }
    return false;
}

pub fn getSize(binarySlice: []const u8, allocator: std.mem.Allocator) !u32 {
    var jotParser: JotParser = JotParserMake(allocator);
    defer jotParser.deinit();

    debugPrint("\t\"{s}\"\n\t=jotParse> ", .{binarySlice});
    const jotExpr = try jotParser.parse(binarySlice);
    if (debugPrintON) jotExpr.traversePrint();
    return jotExpr.size();
}

pub fn streamBinaryStrsRange(
    // allocator: std.mem.Allocator, //
    a: BigIntManaged,
    b: BigIntManaged,
    comptime foreach: *const fn (*BigIntManaged, []const u8, args: anytype) error{ Yield, Else }!void,
    args: anytype,
    progressPrint: bool,
) !void {
    var fmtBuffer: [400]u8 = undefined;

    var i: BigIntManaged = try a.clone();
    defer i.deinit();

    var progressI: BigIntManaged = try b.clone();
    defer progressI.deinit();

    var r: BigIntManaged = try b.clone();
    defer r.deinit();

    var ten: BigIntManaged = try b.clone();
    defer ten.deinit();
    try ten.set(10);

    if (progressPrint) {
        try progressI.divFloor(&r, &b, &ten);
        try r.copy(progressI.toConst());
    }

    var onwards = true;
    var percent: u32 = 10;
    while (onwards) {
        if (progressPrint and progressI.eql(i)) {
            try progressI.add(&progressI, &r);
            std.debug.print("{d}% reached\n", .{percent});
            percent += 10;
        }
        @memset(&fmtBuffer, 0);

        const binarySlice = try std.fmt.bufPrint(&fmtBuffer, "{b}", .{i});

        // std.debug.print("\n{s}", .{binarySlice});

        try foreach(&i, binarySlice, args);

        onwards = switch (i.order(b)) {
            std.math.Order.eq => false,
            std.math.Order.gt => false,
            std.math.Order.lt => true,
        };
        if (onwards) {
            try i.addScalar(&i, 1);
        }
    }
}

const exp6HashmapKey = struct {
    size: u32, //
    steps: u32,
};
//map<size -> map<steps -> haltedCount >>
const exp6stepHaltedHashmapType = std.AutoHashMap(u32, u32);
const exp6hashmapType = std.AutoHashMap(u32, exp6stepHaltedHashmapType);

const exp6SizeTypeSubHashmap = std.AutoHashMap(u32, u32);
const exp6SizeTypeHashmap = std.AutoHashMap(SizeTypeDivTen, *exp6SizeTypeSubHashmap);
const exp6args = struct {
    allocator: std.mem.Allocator, //
    hashmap: *exp6hashmapType,
    totalsHashmap: *exp6hashmapType, // size -> <steps -> total>
    haltedStepHashmap: *std.AutoHashMap(u32, u32),
    sizeTypedHashmap: *exp6SizeTypeHashmap, // sizeType -> (size->haltingStep)
    chosenSteps: u32,
    chosenSize: ?u32,
    yieldOnFirstHalt: bool = false,
};
//TODO make exp6 or something, exp5 works by going from 0 to the step that leads to halting,
// exp6 works by finding the start(no halting) and end steps(all halting) and picks a number of data points in between.
// in theory this will be helpful for later on the really large programs so we dont take forever finding our statistics.
fn exp6(_: *BigIntManaged, input: []const u8, argz: anytype) error{ Yield, Else }!void {
    const args: exp6args = argz;

    const size = getSize(input, args.allocator) catch return error.Else;

    const haltedStepMaybe = args.haltedStepHashmap.get(size);
    if (args.yieldOnFirstHalt) if (haltedStepMaybe) |_| return;

    if (args.chosenSize) |chosenSize| if (size != chosenSize) return;

    var t3Arena = std.heap.ArenaAllocator.init(args.allocator);
    defer t3Arena.deinit();

    const traverses = args.chosenSteps;

    // for (1..args.maxSteps) |traverses| {
    const halted = cacaHalted(input, traverses, t3Arena.allocator()) catch return error.Else;

    if (halted) { //halted
        args.haltedStepHashmap.put(size, traverses) catch return error.Else;

        const sizeTypeDivTenMaybe = sizeGetTypeDivTen(size);

        if (sizeTypeDivTenMaybe) |sizeTypeDivTen| {
            const sizeTypedHashmapMaybe = args.sizeTypedHashmap.get(sizeTypeDivTen);
            if (sizeTypedHashmapMaybe) |sizeTypedHashmap| {
                sizeTypedHashmap.put(size, traverses) catch return error.Else;
            } else {
                const sizeTypedHashmap = args.allocator.create(exp6SizeTypeSubHashmap) catch return error.Else;
                sizeTypedHashmap.* = exp6SizeTypeSubHashmap.init(args.allocator);

                args.sizeTypedHashmap.put(sizeTypeDivTen, sizeTypedHashmap) catch return error.Else;

                sizeTypedHashmap.put(size, traverses) catch return error.Else;
            }
        }

        const thing = args.hashmap.getOrPut(size) catch return error.Else;
        if (!thing.found_existing) {
            thing.value_ptr.* = exp6stepHaltedHashmapType.init(args.allocator);
        }

        const haltedCountRes = thing.value_ptr.*.getOrPut(traverses) catch return error.Else;
        if (haltedCountRes.found_existing) {
            haltedCountRes.value_ptr.* += 1;
        } else {
            haltedCountRes.value_ptr.* = 1;
        }

        if (args.yieldOnFirstHalt) return error.Yield;
    } else {}
    const thing = args.totalsHashmap.getOrPut(size) catch return error.Else;
    if (!thing.found_existing) {
        thing.value_ptr.* = exp6stepHaltedHashmapType.init(args.allocator);
    }

    const totalRes = thing.value_ptr.*.getOrPut(@intCast(traverses)) catch return error.Else;
    if (totalRes.found_existing) {
        totalRes.value_ptr.* += 1;
    } else {
        totalRes.value_ptr.* = 1;
    }
    // }
}

pub const binarySearchFirstHaltingStepForSizeCache = std.AutoHashMap(u32, u32);

/// find the first step at which a size halts
pub fn binarySearchFirstHaltingStepForSize(
    sizez: u32, //
    cacheMaybe: ?*binarySearchFirstHaltingStepForSizeCache,
    allocator: std.mem.Allocator,
) !?u32 {
    if (cacheMaybe) |cache| {
        const cachedRes = cache.get(sizez);
        if (cachedRes) |result| {
            return result;
        }
    }
    // const startSize = 5;
    // const lastSize = 200;
    var first = try BigIntManaged.init(allocator);
    var last = try BigIntManaged.init(allocator);
    var size = try BigIntManaged.init(allocator);

    defer size.deinit();
    defer first.deinit();
    defer last.deinit();

    var hashmap = exp6hashmapType.init(allocator);
    defer hashmap.deinit();

    var totalsHashmap = exp6hashmapType.init(allocator);
    defer totalsHashmap.deinit();

    var haltedStepHashmap = std.AutoHashMap(u32, u32).init(allocator);
    defer haltedStepHashmap.deinit();

    var sizeTypedHashmap = exp6SizeTypeHashmap.init(allocator);
    defer sizeTypedHashmap.deinit();

    // for (startSize..lastSize) |chosenSizeUsize| {
    // const chosenSize: u32 = @intCast(chosenSizeUsize);
    const chosenSize: u32 = sizez;

    // std.debug.print("\n", .{});
    try size.set(chosenSize);

    // this only succeeds if the size "exists" because not all numbers correspond to a size
    const indicesSuceed = try firstIndexSeenForProgramSize(&first, &size, allocator) and try lastIndexSeenForProgramSize(&last, &size, allocator);

    if (indicesSuceed) {
        // std.debug.print("trying size {} -> first {d}, last {d}\n", .{ chosenSize, first, last });

        var currentStep: u32 = 1;

        //TODO might not really need this... idk it could be useful small speedup idk
        const sizeTypeMaybe = sizeGetTypeDivTen(chosenSize);
        if (sizeTypeMaybe) |sizeType| {
            // std.debug.print("size type found\n", .{});
            const sizeTypedHaltingHashmapMaybe = sizeTypedHashmap.get(sizeType);
            if (sizeTypedHaltingHashmapMaybe) |sizeTypedHaltingHashmap| {
                // std.debug.print("sizeTypedHashmap found\n", .{});
                var currentBestSize: u32 = 0;
                var currentBestStep: u32 = 1;
                var it = sizeTypedHaltingHashmap.iterator();

                while (it.next()) |entry| {
                    const cmpSize = entry.key_ptr.*;
                    const cmpSteps = entry.value_ptr.*;

                    // std.debug.print("cmping {},{}\n", .{ cmpSize, cmpSteps });

                    if (cmpSize > chosenSize) {
                        continue;
                    } else if (cmpSize == chosenSize) {
                        currentBestStep = cmpSteps;
                        break;
                    } else {
                        const difference = chosenSize - cmpSize;
                        if (difference < (chosenSize - currentBestSize)) {
                            currentBestSize = cmpSize;
                            currentBestStep = cmpSteps;
                        }
                    }
                }

                currentStep = currentBestStep;
            }
        }

        var start: u32 = currentStep;
        var end: u32 = currentStep * 2;
        var middle: u32 = ((end - start) / 2) + start;
        var endHalted = false;
        currentStep = end;

        var findingNonZeroStep = true;
        while (findingNonZeroStep) {
            // std.debug.print("size {}, trying {} steps, S {}, M {}, E {}\n", .{ chosenSize, currentStep, start, middle, end });

            const args: exp6args = .{
                .allocator = allocator, //
                .hashmap = &hashmap,
                .totalsHashmap = &totalsHashmap,
                .haltedStepHashmap = &haltedStepHashmap,
                .sizeTypedHashmap = &sizeTypedHashmap,
                .chosenSize = chosenSize,
                .chosenSteps = currentStep,
                .yieldOnFirstHalt = true, // we are only looking for the first halting, dont keep going after finding it
            };
            streamBinaryStrsRange(first, last, &exp6, args, false) catch |err| {
                if (err == error.Else) {
                    return err;
                }
            };

            const haltedLastMaybe = haltedStepHashmap.get(chosenSize);
            if (endHalted == false) {
                if (haltedLastMaybe) |_| {
                    endHalted = true;
                    // end = middle;
                    middle = ((end - start) / 2) + start;
                    currentStep = middle;
                } else {
                    end = end * 2;
                    middle = ((end - start) / 2) + start;
                    currentStep = end;
                }
            } else {
                if (haltedLastMaybe) |_| {
                    end = middle;
                } else {
                    start = middle;
                }
                middle = ((end - start) / 2) + start;
                currentStep = middle;
            }

            if (end == start + 1) {
                if (haltedLastMaybe != null) {
                    // std.debug.print("{},{}\n", .{ chosenSize, haltedLastMaybe.? });
                    if (cacheMaybe) |cache| {
                        try cache.put(sizez, haltedLastMaybe.?);
                    }
                    return haltedLastMaybe.?;
                } else {
                    try haltedStepHashmap.put(chosenSize, middle + 1);
                    // std.debug.print("{},{}\n", .{ chosenSize, middle + 1 });
                    if (cacheMaybe) |cache| {
                        try cache.put(sizez, middle + 1);
                    }
                    return middle + 1;
                }
                findingNonZeroStep = false;
            }
            _ = haltedStepHashmap.remove(chosenSize);
        }

        // std.debug.print("found halting greater than 0% at step {}\n", .{currentStep});
    }
    // }
    return null;
}

/// find the step at which 99% size halts
pub fn binarySearch99PercentHaltingStepForSize(
    sizez: u32, //
    cacheMaybe: ?*binarySearchFirstHaltingStepForSizeCache,
    allocator: std.mem.Allocator,
) !?u32 {
    if (cacheMaybe) |cache| {
        const cachedRes = cache.get(sizez);
        if (cachedRes) |result| {
            return result;
        }
    }
    // const startSize = 5;
    // const lastSize = 200;
    var first = try BigIntManaged.init(allocator);
    var last = try BigIntManaged.init(allocator);
    var size = try BigIntManaged.init(allocator);

    defer size.deinit();
    defer first.deinit();
    defer last.deinit();

    var hashmap = exp6hashmapType.init(allocator);
    defer hashmap.deinit();

    var totalsHashmap = exp6hashmapType.init(allocator);
    defer totalsHashmap.deinit();

    var haltedStepHashmap = std.AutoHashMap(u32, u32).init(allocator);
    defer haltedStepHashmap.deinit();

    var sizeTypedHashmap = exp6SizeTypeHashmap.init(allocator);
    defer sizeTypedHashmap.deinit();

    // for (startSize..lastSize) |chosenSizeUsize| {
    // const chosenSize: u32 = @intCast(chosenSizeUsize);
    const chosenSize: u32 = sizez;

    // std.debug.print("\n", .{});
    try size.set(chosenSize);

    // this only succeeds if the size "exists" because not all numbers correspond to a size
    const indicesSuceed = try firstIndexSeenForProgramSize(&first, &size, allocator) and try lastIndexSeenForProgramSize(&last, &size, allocator);

    if (indicesSuceed) {
        // std.debug.print("trying size {} -> first {d}, last {d}\n", .{ chosenSize, first, last });

        var currentStep: u32 = 20;

        //TODO might not really need this... idk it could be useful small speedup idk
        // const sizeTypeMaybe = sizeGetTypeDivTen(chosenSize);
        // if (sizeTypeMaybe) |sizeType| {
        //     // std.debug.print("size type found\n", .{});
        //     const sizeTypedHaltingHashmapMaybe = sizeTypedHashmap.get(sizeType);
        //     if (sizeTypedHaltingHashmapMaybe) |sizeTypedHaltingHashmap| {
        //         // std.debug.print("sizeTypedHashmap found\n", .{});
        //         var currentBestSize: u32 = 0;
        //         var currentBestStep: u32 = 1;
        //         var it = sizeTypedHaltingHashmap.iterator();

        //         while (it.next()) |entry| {
        //             const cmpSize = entry.key_ptr.*;
        //             const cmpSteps = entry.value_ptr.*;

        //             // std.debug.print("cmping {},{}\n", .{ cmpSize, cmpSteps });

        //             if (cmpSize > chosenSize) {
        //                 continue;
        //             } else if (cmpSize == chosenSize) {
        //                 currentBestStep = cmpSteps;
        //                 break;
        //             } else {
        //                 const difference = chosenSize - cmpSize;
        //                 if (difference < (chosenSize - currentBestSize)) {
        //                     currentBestSize = cmpSize;
        //                     currentBestStep = cmpSteps;
        //                 }
        //             }
        //         }

        //         currentStep = currentBestStep;
        //     }
        // }

        var start: u32 = currentStep;
        var end: u32 = currentStep * 2;
        var middle: u32 = ((end - start) / 2) + start;
        var end99Halted = false;
        currentStep = end;

        var findingNonZeroStep = true;
        while (findingNonZeroStep) {
            // std.debug.print("size {}, trying {} steps, S {}, M {}, E {}\n", .{ chosenSize, currentStep, start, middle, end });

            const args: exp6args = .{
                .allocator = allocator, //
                .hashmap = &hashmap,
                .totalsHashmap = &totalsHashmap,
                .haltedStepHashmap = &haltedStepHashmap,
                .sizeTypedHashmap = &sizeTypedHashmap,
                .chosenSize = chosenSize,
                .chosenSteps = currentStep,
            };
            streamBinaryStrsRange(first, last, &exp6, args, false) catch |err| {
                if (err == error.Else) {
                    return err;
                }
            };

            var halted99M: ?u32 = null;

            const totalMapM = args.totalsHashmap.get(sizez);
            const haltedMapM = args.hashmap.get(sizez);

            if (totalMapM != null and haltedMapM != null) {
                const haltedM = haltedMapM.?.get(currentStep);
                const totalM = totalMapM.?.get(currentStep);
                if (haltedM != null and totalM != null) {
                    const halted: f64 = haltedM.?;
                    const total: f64 = totalM.?;
                    const freqHalted = halted / total;

                    // std.debug.print("{} halted, {} total, {}% halted\n", .{ halted, total, freqHalted * 100 });

                    const greater99 = freqHalted >= 0.99;
                    if (greater99) halted99M = currentStep;
                }
            }

            if (end99Halted == false) {
                if (halted99M) |_| {
                    end99Halted = true;
                    // end = middle;
                    middle = ((end - start) / 2) + start;
                    currentStep = middle;
                } else {
                    end = end * 2;
                    middle = ((end - start) / 2) + start;
                    currentStep = end;
                }
            } else {
                if (halted99M) |_| {
                    end = middle;
                } else {
                    start = middle;
                }
                middle = ((end - start) / 2) + start;
                currentStep = middle;
            }

            if (end == start + 1) {
                if (halted99M != null) {
                    const result = halted99M.?;
                    // std.debug.print("{},{}\n", .{ chosenSize, result });
                    if (cacheMaybe) |cache| {
                        try cache.put(sizez, result);
                    }
                    return result;
                } else {
                    const result = middle + 1;
                    try haltedStepHashmap.put(chosenSize, result);
                    // std.debug.print("{},{}\n", .{ chosenSize, result });
                    if (cacheMaybe) |cache| {
                        try cache.put(sizez, result);
                    }
                    return result;
                }
                findingNonZeroStep = false;
            }
            _ = haltedStepHashmap.remove(chosenSize);
        }

        // std.debug.print("found halting >= than 99% at step {}\n", .{currentStep});
    }
    // }
    return null;
}

// binary str -> jot parse -> reduce -> binary str
pub fn caca(binarySlice: []const u8, maxTraverses: ?u32, allocator: std.mem.Allocator) !SizeAndOutput {
    var interpreter = LambdaInterpreter.init(allocator);
    var jotParser: JotParser = JotParserMake(allocator);
    defer jotParser.deinit();

    debugPrint("\t\"{s}\"\n\t=jotParse> ", .{binarySlice});
    const jotExpr = try jotParser.parse(binarySlice);
    if (debugPrintON) jotExpr.traversePrint();

    if (interpreter.reduceUntilCantReduce(jotExpr, maxTraverses, null, true)) |reducedExpr| {
        debugPrint("\n\t=reduce> ", .{});
        if (debugPrintON) reducedExpr.traversePrint();
        const blcSliceString2 = try loadBlcExpressionToSlice(reducedExpr, allocator);
        debugPrint("\n\t=blc> ", .{});
        debugPrint("\"{s}\"", .{blcSliceString2});
        return .{ .inputSize = jotExpr.size(), .output = blcSliceString2 };
    } else |err| switch (err) {
        else => {
            debugPrint("\n\t=reduce> {}", .{err});
            return err;
        },
    }
}

pub fn cacaHalted(binarySlice: []const u8, maxTraverses: ?u32, allocator: std.mem.Allocator) !bool {
    var interpreter = LambdaInterpreter.init(allocator);
    var jotParser: JotParser = JotParserMake(allocator);
    defer jotParser.deinit();

    debugPrint("\t\"{s}\"\n\t=jotParse> ", .{binarySlice});
    const jotExpr = try jotParser.parse(binarySlice);
    if (debugPrintON) jotExpr.traversePrint();

    if (interpreter.reduceUntilCantReduce(jotExpr, maxTraverses, null, true)) |reducedExpr| {
        debugPrint("\n\t=reduce> ", .{});
        if (debugPrintON) reducedExpr.traversePrint();
        const blcSliceString2 = try loadBlcExpressionToSlice(reducedExpr, allocator);
        debugPrint("\n\t=blc> ", .{});
        debugPrint("\"{s}\"", .{blcSliceString2});
        // return .{ .inputSize = jotExpr.size(), .output = blcSliceString2 };
        return true;
    } else |err| switch (err) {
        else => {
            debugPrint("\n\t=reduce> {}", .{err});
            if (err == error.MAX_TRAVERSES) return false else return err;
        },
    }
}
