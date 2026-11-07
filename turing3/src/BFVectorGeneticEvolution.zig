const BigFloat = @import("BFP").BigFloat(.{ .Significand = f64, .Exponent = i64 });
const std = @import("std");

pub fn BFVectorEvolutionContext(
    genomeSize: comptime_int,
    fitnessFunc: fn ([]BigFloat) f64, //

    mutateChance: comptime_float,
    crossoverChance: comptime_float,
    iterations: comptime_int,
    populationSize: comptime_int,
    selectionTopPercent: comptime_float, //

    randomSeed: comptime_int,
) type {
    const BFGenome: type = [genomeSize]BigFloat;

    const Individual: type = struct { genome: BFGenome, fitness: f64 };

    return struct {
        const Self = @This();

        population: [populationSize]Individual = undefined, //
        prng: std.Random.DefaultPrng = .init(@intCast(randomSeed)),
        initialized: bool = false,

        fn randFloatRange(self: *Self, left: f64, right: f64) f64 {
            return std.math.clamp((self.prng.random().float(f64) * (right - left)) - left, left, right);
        }

        fn mutate(self: *Self, genome: *BFGenome) void {
            for (0..genomeSize) |i| {
                const gene: *BigFloat = &genome[i];
                if (self.prng.random().float(f64) <= mutateChance) {
                    const mutateOp = self.prng.random().intRangeAtMost(u32, 0, 2);
                    switch (mutateOp) {
                        0 => {
                            const num = self.randFloatRange(-100, 100);
                            gene.* = gene.add(BigFloat.init(num));
                        },
                        1 => {
                            const num = self.randFloatRange(-2, 2);
                            gene.* = gene.mul(BigFloat.init(num));
                        },
                        2 => {
                            const num = self.randFloatRange(0.001, 2);
                            gene.* = gene.pow(BigFloat.init(num));
                        },
                        else => unreachable,
                    }
                }
            }
        }

        fn crossover(self: *Self, a: *BFGenome, b: *BFGenome) void {
            for (0..genomeSize) |i| {
                const gene1: *BigFloat = &a[i];
                const gene2: *BigFloat = &b[i];
                if (self.prng.random().float(f64) <= crossoverChance) {
                    //swap the genes
                    const temp = gene1.*;
                    gene1.* = gene2.*;
                    gene2.* = temp;
                }
            }
        }

        fn cloneIndivual(source: *Individual, destination: *Individual) void {
            destination.*.fitness = source.*.fitness;
            destination.*.genome = source.*.genome;
        }

        fn fitnessCmpDesc(_: void, a: Individual, b: Individual) bool {
            return a.fitness > b.fitness;
        }

        pub fn fillWithRandom(self: *Self, individual: *Individual) void {
            individual.*.fitness = -999;
            for (0..genomeSize) |geneI| {
                individual.*.genome[geneI] = BigFloat.init(self.prng.random().float(f64));
            }
        }

        pub fn getPopulation(self: *Self) [populationSize]Individual {
            return self.population;
        }

        pub fn epoch(self: *Self) void {
            if (self.initialized == false) {
                self.initialized = true;
                for (0..populationSize) |individualI| {
                    const individual = &self.population[individualI];
                    self.fillWithRandom(individual);
                }
            }
            for (0..iterations) |_| {
                //fitness evalutation
                for (0..self.population.len) |populationI| {
                    const fitness: *f64 = &self.population[populationI].fitness;
                    const genome: []BigFloat = &self.population[populationI].genome;

                    fitness.* = fitnessFunc(genome);
                }

                //sort by fitness
                std.mem.sort(Individual, &self.population, {}, fitnessCmpDesc);

                //selection
                const cutoffI: u32 = (populationSize * selectionTopPercent);

                //clone the top performers into the bottom performers
                for (cutoffI..populationSize) |individualI| {
                    const topIndividualChoice = self.prng.random().intRangeAtMost(u32, 0, cutoffI - 1);
                    cloneIndivual( //
                        &self.population[topIndividualChoice], //
                        &self.population[individualI]);
                }

                //crossover the bottom performers with eachother
                for (cutoffI..populationSize) |individualI| {
                    const bottomIndividualChoice = self.prng.random().intRangeAtMost(u32, cutoffI, populationSize - 1);
                    self.crossover( //
                        &self.population[bottomIndividualChoice].genome, //
                        &self.population[individualI].genome);
                }

                //mutation the bottom performers
                for (cutoffI..self.population.len) |populationI| {
                    const genome: *BFGenome = &self.population[populationI].genome;

                    self.mutate(genome);
                }
            }
        }
    };
}

pub fn fitnessTestFunc1(genome: []BigFloat) f64 {
    var sum: f64 = 0.0;
    for (0..genome.len) |i| {
        const gene = genome[i];
        sum += gene.toFloat(f64);
    }
    return sum;
}

pub fn fitnessTestFunc2(genome: []BigFloat) f64 {
    var sum: f64 = 0.0;
    for (0..genome.len) |i| {
        const gene = genome[i];
        const iF: f64 = @floatFromInt(i);
        sum -= @abs((iF - gene.toFloat(f64)));
    }
    return sum;
}

pub fn fitnessTestFunc3(genome: []BigFloat) f64 {
    return -rosenbrock(genome[0].toFloat(f64), genome[1].toFloat(f64));
}

/// Global minimum is at (1, 1) where f(x, y) = 0
fn rosenbrock(x: f64, y: f64) f64 {
    const a = 1.0;
    const b = 100.0;
    return (a - x) * (a - x) + b * (y - x * x) * (y - x * x);
}
