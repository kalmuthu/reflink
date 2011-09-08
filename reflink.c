
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <err.h>
#include <errno.h>
#include <stdint.h>

static inline int
btrfs_reflink (const char *src_path, const char *dest_path)
{
  int src_fd, dest_fd;
  src_fd = open(src_path, O_RDONLY);
  if (src_fd < 0)
    err(1, "%s", src_path);
  struct stat buf;
  if (fstat(src_fd, &buf) < 0)
    err(1, "%s", src_path);
  dest_fd = open(dest_path, O_WRONLY | O_CREAT | O_EXCL, buf.st_mode);
  if (dest_fd < 0) {
    if (errno == EEXIST)
      err(1, "%s", dest_path);
    return -1;
  }
  return ioctl(dest_fd, _IOW(0x94, 9, int), src_fd);
}

static inline int
ocfs2_reflink (const char *src_path, const char *dest_path)
{
  struct args {
    uint64_t src_path;
    uint64_t dest_path;
    uint64_t preserve;
  } args = {
    (uint64_t) src_path, (uint64_t) dest_path, 1
  };
  int fd = open(src_path, O_RDONLY);
  if (fd < 0) return -1;
  return ioctl(fd, _IOW(0x6f, 4, struct args), &args);
}

int main (int argc, char **argv)
{
	if (argc != 3) errx(1, "Usage: %s SRC DST", argv[0]);
	if (!btrfs_reflink(argv[1], argv[2]))
	  return 0;
	if (!ocfs2_reflink(argv[1], argv[2]))
	  return 0;
	int tmp_errno = errno;
	errno = 0;
	if (unlink(argv[2]))
	  if (errno != ENOENT) {
	    warn("unlinking %s failed", argv[2]);
	    tmp_errno = errno;
	  }
	errno = tmp_errno;
  err(1, "failed");
}

