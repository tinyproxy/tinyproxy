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

    const version: []u8 = try getVersionFromGit(allocator);
    const package = "tinyproxy";
    const package_name = "Tinyproxy";
    const package_tarname = package;
    var package_string_buf: [128:0]u8 = undefined;
    var package_string = try std.fmt.bufPrint(&package_string_buf, "{s} {s}", .{ package_name, version });
    // const package_string: []u8 = try allocator.alloc(u8, package_name.len + 1 + version.len); // package_name ++ version;
    // package_string[0..package_name.len].* = package_name.*;
    // package_string[package_name.len] = ' ';
    // package_string[package_name.len + 1 .. package_string.len].* = version.*;

    const config_h = b.addConfigHeader(.{
        .style = .blank,
    }, .{
        .VERSION = version,
        .TINYPROXY_VERSION = version,
        .XTINYPROXY_ENABLE = 1,
        .FILTER_ENABLE = 1,
        .UPSTREAM_SUPPORT = 1,
        .REVERSE_SUPPORT = 1,
        .TRANSPARENT_PROXY = 1,
        .PACKAGE = package,
        .PACKAGE_BUGREPORT = "https://tinyproxy.github.io/",

        .PACKAGE_NAME = package_name,
        .PACKAGE_STRING = package_string,

        .PACKAGE_TARNAME = package_tarname,

        .PACKAGE_URL = "",

        .PACKAGE_VERSION = version,

        // /* Define to 1 if you can safely include both <sys/time.h> and <time.h>. This
        //    macro is obsolete. */
        .TIME_WITH_SYS_TIME = 1,

        // /* This controls remote proxy stats display. */
        .TINYPROXY_STATHOST = "tinyproxy.stats",
    });

    const exe = b.addExecutable(.{
        .name = "tinyproxy",
        .target = target,
        .optimize = optimize,
    });
    exe.addConfigHeader(config_h);
    exe.addIncludePath(.{ .path = "src/" });

    exe.defineCMacro("HAVE_CONFIG_H", null);
    exe.defineCMacro("SYSCONFDIR", "\"/etc\"");
    // exe.defineCMacro("LOCALSTATEDIR", "")

    if (target.isWindows()) {
        exe.defineCMacro("_CRT_SECURE_NO_DEPRECATE", "1");
        exe.defineCMacro("HAVE_LIBCRYPT32", null);
        exe.defineCMacro("HAVE_WINSOCK2_H", null);
        exe.defineCMacro("HAVE_IOCTLSOCKET", null);
        exe.defineCMacro("HAVE_SELECT", null);
        exe.defineCMacro("LIBSSH2_DH_GEX_NEW", "1");

        if (target.getAbi().isGnu()) {
            exe.defineCMacro("HAVE_UNISTD_H", null);
            exe.defineCMacro("HAVE_INTTYPES_H", null);
            exe.defineCMacro("HAVE_SYS_TIME_H", null);
            exe.defineCMacro("HAVE_GETTIMEOFDAY", null);
        }
    } else {
        exe.defineCMacro("HAVE_UNISTD_H", null);
        exe.defineCMacro("HAVE_INTTYPES_H", null);
        exe.defineCMacro("HAVE_STDLIB_H", null);
        exe.defineCMacro("HAVE_SYS_SELECT_H", null);
        exe.defineCMacro("HAVE_SYS_UIO_H", null);
        exe.defineCMacro("HAVE_SYS_SOCKET_H", null);
        exe.defineCMacro("HAVE_SYS_IOCTL_H", null);
        exe.defineCMacro("HAVE_SYS_TIME_H", null);
        exe.defineCMacro("HAVE_SYS_UN_H", null);
        exe.defineCMacro("HAVE_LONGLONG", null);
        exe.defineCMacro("HAVE_GETTIMEOFDAY", null);
        exe.defineCMacro("HAVE_INET_ADDR", null);
        exe.defineCMacro("HAVE_POLL", null);
        exe.defineCMacro("HAVE_SELECT", null);
        exe.defineCMacro("HAVE_SOCKET", null);
        exe.defineCMacro("HAVE_STRTOLL", null);
        exe.defineCMacro("HAVE_SNPRINTF", null);
        exe.defineCMacro("HAVE_O_NONBLOCK", null);
        exe.defineCMacro("HAVE_SYSEXITS_H", null);
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
