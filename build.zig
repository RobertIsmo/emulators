const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const cbuild = b.option(bool, "cbuild", "Build the C Sources") orelse false;

    const chip8 = b.option(bool, "chip8", "Build the CHIP-8 Emulator") orelse false;

    if (chip8) {
        if (cbuild) build_c_sources(b, "CHIP-8_c", "chip8/c/", target, optimize);
    }
}

pub fn build_c_sources(b: *std.Build, name: []const u8, comptime dir: []const u8, target: std.Build.ResolvedTarget, optimize: std.builtin.OptimizeMode) void {
    const exe = b.addExecutable(.{
        .name = name,
        .target = target,
        .optimize = optimize,
    });

    exe.linkLibC();
    exe.addIncludePath(b.path(dir));
    exe.addCSourceFile(.{ .file = b.path(dir ++ "main.c") });
    b.installArtifact(exe);
}
