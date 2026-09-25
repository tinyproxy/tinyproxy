const std = @import("std");

fn getVersionFromFile() ![]u8 {
    const file = try std.fs.cwd().openFile(
        "VERSION",
        .{},
    );
    defer file.close();

    const len = 32;
    var buffer: [len]u8 = undefined;
    try file.seekTo(0);
    const bytes_read = try file.readAll(&buffer);
    return buffer[0..@min(len, bytes_read)];
}

fn getVersionFromGit(allocator: std.mem.Allocator) ![]u8 {
    const args = [_][]const u8{
        "bash",
        "scripts/version.sh",
    };

    const t = try std.ChildProcess.exec(.{
        .allocator = allocator,
        .argv = &args,
        .max_output_bytes = 128,
    });
    defer {
        allocator.free(t.stdout);
        allocator.free(t.stderr);
    }

    const result = try allocator.alloc(u8, t.stdout.len);
    @memcpy(result, t.stdout);
    return result;
}

pub fn build(b: *std.Build) !void {
    const optimize = b.standardOptimizeOption(.{});
    const target = b.standardTargetOptions(.{});

    const allocator = b.allocator;

    const gperf = b.option(bool, "gperf", "Whether you have gperf installed for faster config parsing") orelse false;
    const xtinyproxy_enable = b.option(bool, "xtinyproxy_header", "Include peer's IP address in a XTinyproxy header") orelse false;
    const debug_enable = b.option(bool, "debug_enable", "Compile in DEBUG program mode") orelse false;
    const sysconfigdir = b.option([]const u8, "sysconfigdir", "System configuration path (/etc)") orelse "/etc";

    var sysconfigdir_macro: []u8 = try allocator.alloc(u8, sysconfigdir.len + 10);
    defer allocator.free(sysconfigdir_macro);
    sysconfigdir_macro = try std.fmt.bufPrintZ(sysconfigdir_macro, "\"{s}\"", .{sysconfigdir});

    const version: []u8 = try getVersionFromGit(allocator);
    const package = "tinyproxy";
    const package_name = "Tinyproxy";
    const package_tarname = package;
    var package_string_buf: [128:0]u8 = undefined;
    var package_string = try std.fmt.bufPrint(&package_string_buf, "{s} {s}", .{ package_name, version });

    const config_h = b.addConfigHeader(.{
        .style = .blank,
    }, .{
        // /* Version number of package */
        .VERSION = version,
        // /* Tinyproxy version number */
        .TINYPROXY_VERSION = version,
        // /* Name of package */
        .PACKAGE = package,
        // /* Define to the address where bug reports for this package should be sent. */
        .PACKAGE_BUGREPORT = "https://tinyproxy.github.io/",
        // /* Define to the full name of this package. */
        .PACKAGE_NAME = package_name,
        // /* Define to the full name and version of this package. */
        .PACKAGE_STRING = package_string,
        // /* Define to the one symbol short name of this package. */
        .PACKAGE_TARNAME = package_tarname,
        // /* Define to the home page for this package. */
        .PACKAGE_URL = "",
        // /* Define to the version of this package. */
        .PACKAGE_VERSION = version,
        // /* This controls remote proxy stats display. */
        .TINYPROXY_STATHOST = "tinyproxy.stats",

        // /* Include support for connecting to an upstream proxy. */
        .UPSTREAM_SUPPORT = 1,
        // /* Include support for reverse proxy. */
        .REVERSE_SUPPORT = 1,
        // /* Include support for using tinyproxy as a transparent proxy. */
        .TRANSPARENT_PROXY = 1,
        // /* Defined if you would like filtering code included. */
        .FILTER_ENABLE = 1,

        // /* Define to 1 if you can safely include both <sys/time.h> and <time.h>. This
        //    macro is obsolete. */
        .TIME_WITH_SYS_TIME = 1,
        // /*
        //  * On systems which don't support ftruncate() the best we can
        //  * do is to close the file and reopen it in create mode, which
        //  * unfortunately leads to a race condition, however "systems
        //  * which don't support ftruncate()" is pretty much SCO only,
        //  * and if you're using that you deserve what you get.
        //  * ("Little sympathy has been extended")
        //  */
        .HAVE_FTRUNCATE = 1,
        // /* Define to 1 if you have the <poll.h> header file. */
        .HAVE_POLL_H = 1,
        // /* Define to 1 if you have the `setgroups' function. */
        .HAVE_SETGROUPS = 1,
        // /* Define to 1 if you have the <values.h> header file. */
        .HAVE_VALUES_H = 1,
        // /* Define to 1 if you have the <sys/ioctl.h> header file. */
        .HAVE_SYS_IOCTL_H = 1,
        // /* Define to 1 if you have the <alloca.h> header file. */
        .HAVE_ALLOCA_H = 1,
        // /* Define to 1 if you have the <memory.h> header file. */
        .HAVE_MEMORY_H = 1,
        // /* Define to 1 if you have the <malloc.h> header file. */
        .HAVE_MALLOC_H = 1,
        // /* Define to 1 if you have the <sysexits.h> header file. */
        .HAVE_SYSEXITS_H = 1,
    });

    const exe = b.addExecutable(.{
        .name = "tinyproxy",
        .target = target,
        .optimize = optimize,
    });
    exe.addConfigHeader(config_h);
    exe.addIncludePath(.{ .path = "src/" });

    exe.defineCMacro("HAVE_CONFIG_H", null);

    exe.defineCMacro("SYSCONFDIR", sysconfigdir_macro);

    // /* Whether you have gperf installed for faster config parsing. */
    if (gperf) {
        exe.defineCMacro("HAVE_GPERF", null);
    }
    // /* Define if you want to have the peer's IP address included in a XTinyproxy
    //    header sent to the server. */
    if (xtinyproxy_enable) {
        exe.defineCMacro("XTINYPROXY_ENABLE", null);
    }
    if (!debug_enable) {
        exe.defineCMacro("NDEBUG", null);
    }

    exe.addCSourceFiles(srcs, &.{});
    exe.linkLibC();
    b.installArtifact(exe);
}

const srcs = &.{
    "src/hostspec.c",
    "src/acl.c",
    "src/anonymous.c",
    "src/buffer.c",
    "src/child.c",
    "src/conf-tokens.c",
    "src/conf.c",
    "src/conns.c",
    "src/daemon.c",
    "src/heap.c",
    "src/html-error.c",
    "src/http-message.c",
    "src/log.c",
    "src/network.c",
    "src/reqs.c",
    "src/sock.c",
    "src/stats.c",
    "src/text.c",
    "src/main.c",
    "src/utils.c",
    "src/upstream.c",
    "src/basicauth.c",
    "src/base64.c",
    "src/sblist.c",
    "src/hsearch.c",
    "src/orderedmap.c",
    "src/loop.c",
    "src/mypoll.c",
    "src/connect-ports.c",
    "src/filter.c",
    "src/reverse-proxy.c",
    "src/transparent-proxy.c",
};
