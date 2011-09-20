
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <err.h>
#include <errno.h>
#include <stdint.h>

#if defined(_IOW) && !defined(BTRFS_IOC_CLONE)
#define BTRFS_IOC_CLONE _IOW(0x94, 9, int)
#endif

int btrfs_reflink (const char *src_path, const char *dest_path)
{
  int src_fd;
  src_fd = open(src_path, O_RDONLY);
  if (src_fd < 0)
    err(1, "%s", src_path);
  struct stat buf;
  if (fstat(src_fd, &buf) < 0)
    err(1, "%s", src_path);
  int dest_fd = open(dest_path, O_WRONLY | O_CREAT | O_EXCL, buf.st_mode);
  if (dest_fd < 0) {
    if (errno == EEXIST)
      err(1, "%s", dest_path);
    close(src_fd);
    return -1;
  }
  int r = ioctl(dest_fd, BTRFS_IOC_CLONE, src_fd);
  if (r < 0)
    close(src_fd), close(dest_fd), unlink(dest_path);
  return r;
}

#if defined(_IOW) && !defined(OCFS2_IOC_REFLINK)
struct reflink_arguments {
  uint64_t old_path;
  uint64_t new_path;
  uint64_t preserve;
};
# define OCFS2_IOC_REFLINK       _IOW('o', 4, struct reflink_arguments)
#endif

int ocfs2_reflink (const char *src_path, const char *dest_path)
{
  struct reflink_arguments args = {
    (uint64_t) src_path, (uint64_t) dest_path, 1
  };
  int fd = open(src_path, O_RDONLY);
  if (fd < 0)
    return -1;
  int r = ioctl(fd, OCFS2_IOC_REFLINK, &args);
  if (r < 0)
    close(fd);
  return r;
}

int main (int argc, char **argv)
{
	if (argc != 3)
	  errx(1, "Usage: %s SRC DST", argv[0]);
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

