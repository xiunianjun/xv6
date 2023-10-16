#define O_RDONLY  0x000
#define O_WRONLY  0x001
#define O_RDWR    0x002
#define O_CREATE  0x200
#define O_TRUNC   0x400
// 意为只用获取软链接文件本身，而不用顺着软链接去找它的target文件
#define O_NOFOLLOW 0x100