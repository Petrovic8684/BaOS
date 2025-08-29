import sys, os, struct, math

MAX_NAME = 16
MAX_DIRS_PER_DIR = 8
MAX_FILES_PER_DIR = 16

FS_MAGIC = 0x46535953  # 'FSYS'

MAX_DIRS  = 64
MAX_FILES = 128

SUPERBLOCK_LBA       = 2048
DIR_TABLE_START_LBA  = SUPERBLOCK_LBA + 1
DIR_TABLE_SECTORS    = MAX_DIRS
FILE_TABLE_START_LBA = DIR_TABLE_START_LBA + DIR_TABLE_SECTORS
FILE_TABLE_SECTORS   = MAX_FILES
DATA_START_LBA       = FILE_TABLE_START_LBA + FILE_TABLE_SECTORS

SECTOR_SIZE = 512

def read_sector(f, lba):
    f.seek(lba * SECTOR_SIZE)
    return f.read(SECTOR_SIZE)

def write_sector(f, lba, data):
    if len(data) > SECTOR_SIZE:
        raise Exception("sector too large")
    f.seek(lba * SECTOR_SIZE)
    f.write(data + b'\x00' * (SECTOR_SIZE - len(data)))
    f.flush()

def find_free(bitmap):
    for i,b in enumerate(bitmap):
        if b == 0: return i
    return -1

def parse_super(buf):
    if len(buf) < 28: return None
    (magic, max_dirs, max_files, dir_table_start, file_table_start, data_start, root_dir_lba) = struct.unpack_from('<7I', buf, 0)
    dir_bitmap = buf[28:28+MAX_DIRS]
    file_bitmap = buf[28+MAX_DIRS:28+MAX_DIRS+MAX_FILES]
    return {
        'magic': magic, 'max_dirs': max_dirs, 'max_files': max_files,
        'dir_table_start': dir_table_start, 'file_table_start': file_table_start,
        'data_start': data_start, 'root_dir_lba': root_dir_lba,
        'dir_bitmap': bytearray(dir_bitmap), 'file_bitmap': bytearray(file_bitmap)
    }

def build_super(s):
    buf = bytearray(SECTOR_SIZE)
    struct.pack_into('<7I', buf, 0,
                     s['magic'], s['max_dirs'], s['max_files'],
                     s['dir_table_start'], s['file_table_start'], s['data_start'],
                     s['root_dir_lba'])
    buf[28:28+MAX_DIRS] = bytes(s['dir_bitmap'])
    buf[28+MAX_DIRS:28+MAX_DIRS+MAX_FILES] = bytes(s['file_bitmap'])
    return bytes(buf)

def parse_dir(buf):
    name = buf[0:MAX_NAME].split(b'\x00',1)[0].decode('ascii',errors='ignore')
    offs = MAX_NAME
    parent_lba = struct.unpack_from('<I', buf, offs)[0]
    offs += 4
    dirs = list(struct.unpack_from('<' + 'I'*MAX_DIRS_PER_DIR, buf, offs))
    offs += 4*MAX_DIRS_PER_DIR
    files = list(struct.unpack_from('<' + 'I'*MAX_FILES_PER_DIR, buf, offs))
    offs += 4*MAX_FILES_PER_DIR
    dir_count = buf[offs]
    file_count = buf[offs+1]
    return {'name': name, 'parent_lba': parent_lba, 'dir_count': dir_count, 'file_count': file_count, 'dirs_lba': dirs, 'files_lba': files}

def build_dir(d):
    buf = bytearray(SECTOR_SIZE)
    bname = d['name'].encode('ascii')[:MAX_NAME-1]
    buf[0:len(bname)] = bname
    offs = MAX_NAME
    struct.pack_into('<I', buf, offs, d.get('parent_lba', 0))
    offs += 4
    for i in range(MAX_DIRS_PER_DIR):
        val = d['dirs_lba'][i] if i < len(d['dirs_lba']) else 0
        struct.pack_into('<I', buf, offs + 4*i, val)
    offs += 4*MAX_DIRS_PER_DIR
    for i in range(MAX_FILES_PER_DIR):
        val = d['files_lba'][i] if i < len(d['files_lba']) else 0
        struct.pack_into('<I', buf, offs + 4*i, val)
    offs += 4*MAX_FILES_PER_DIR
    buf[offs] = d.get('dir_count', 0) & 0xFF
    buf[offs+1] = d.get('file_count', 0) & 0xFF
    return bytes(buf)

def parse_file(buf):
    name = buf[0:MAX_NAME].split(b'\x00',1)[0].decode('ascii',errors='ignore')
    offs = MAX_NAME
    size, data_lba = struct.unpack_from('<2I', buf, offs)
    return {'name': name, 'size': size, 'data_lba': data_lba}

def build_file(f):
    buf = bytearray(SECTOR_SIZE)
    bname = f['name'].encode('ascii')[:MAX_NAME-1]
    buf[0:len(bname)] = bname
    struct.pack_into('<2I', buf, MAX_NAME, f['size'], f['data_lba'])
    return bytes(buf)

def write_file_data(fh, data_lba, data):
    off = 0
    total = len(data)
    sector = data_lba
    while off < total:
        chunk = data[off:off+SECTOR_SIZE]
        write_sector(fh, sector, chunk)
        off += SECTOR_SIZE
        sector += 1

def ensure_fs(fh):
    supbuf = read_sector(fh, SUPERBLOCK_LBA)
    sp = parse_super(supbuf)
    if not sp or sp['magic'] != FS_MAGIC:
        print("No valid super, formatting FS (using provided constants)")
        sp = {
            'magic': FS_MAGIC,
            'max_dirs': MAX_DIRS,
            'max_files': MAX_FILES,
            'dir_table_start': DIR_TABLE_START_LBA,
            'file_table_start': FILE_TABLE_START_LBA,
            'data_start': DATA_START_LBA,
            'root_dir_lba': DIR_TABLE_START_LBA,
            'dir_bitmap': bytearray([0]*MAX_DIRS),
            'file_bitmap': bytearray([0]*MAX_FILES)
        }
        root_idx = 0
        root_lba = DIR_TABLE_START_LBA + root_idx
        root = {'name': '/', 'parent_lba': root_lba, 'dir_count': 0, 'file_count': 0,
                'dirs_lba':[0]*MAX_DIRS_PER_DIR, 'files_lba':[0]*MAX_FILES_PER_DIR}
        write_sector(fh, root_lba, build_dir(root))
        sp['dir_bitmap'][root_idx] = 1
        write_sector(fh, SUPERBLOCK_LBA, build_super(sp))
    return sp

def main():
    if len(sys.argv) < 3:
        print("usage: mkfs_inject.py <img> <bin1> [bin2 ...]")
        return
    img = sys.argv[1]
    bins = sys.argv[2:]
    with open(img, "r+b") as fh:
        sp = ensure_fs(fh)

        root_lba = sp['root_dir_lba']
        root = parse_dir(read_sector(fh, root_lba))

        prog_dir_lba = 0
        for i in range(root['dir_count']):
            dlba = root['dirs_lba'][i]
            d = parse_dir(read_sector(fh, dlba))
            if d['name'] == "programs":
                prog_dir_lba = dlba
                break

        if prog_dir_lba == 0:
            new_idx = find_free(sp['dir_bitmap'])
            if new_idx < 0: raise Exception("no dir slots")
            new_lba = DIR_TABLE_START_LBA + new_idx
            newdir = {'name': 'programs', 'parent_lba': root_lba, 'dir_count': 0, 'file_count': 0,
                      'dirs_lba':[0]*MAX_DIRS_PER_DIR, 'files_lba':[0]*MAX_FILES_PER_DIR}
            write_sector(fh, new_lba, build_dir(newdir))
            sp['dir_bitmap'][new_idx] = 1
            root['dirs_lba'][root['dir_count']] = new_lba
            root['dir_count'] += 1
            write_sector(fh, root_lba, build_dir(root))
            write_sector(fh, SUPERBLOCK_LBA, build_super(sp))
            prog_dir_lba = new_lba
            print("Created /programs at LBA", new_lba)

        prog_dir = parse_dir(read_sector(fh, prog_dir_lba))

        used = set()
        for i in range(MAX_FILES):
            if sp['file_bitmap'][i]:
                flba = FILE_TABLE_START_LBA + i
                finfo = parse_file(read_sector(fh, flba))
                if finfo['data_lba'] != 0 and finfo['size'] > 0:
                    sectors = (finfo['size'] + SECTOR_SIZE - 1) // SECTOR_SIZE
                    for s in range(finfo['data_lba'], finfo['data_lba'] + sectors):
                        used.add(s)

        for b in bins:
            name = os.path.basename(b)
            if name.lower().endswith('.bin'):
                name = name[:-4]
            if len(name) >= MAX_NAME:
                raise Exception("name too long: " + name)
            dup = False
            for i in range(prog_dir['file_count']):
                flba = prog_dir['files_lba'][i]
                fentry = parse_file(read_sector(fh, flba))
                if fentry['name'] == name:
                    dup = True
                    break
            if dup:
                print("Skipping (already exists):", name)
                continue

            if prog_dir['file_count'] >= MAX_FILES_PER_DIR:
                raise Exception("/programs directory is full")

            with open(b, "rb") as bf:
                data = bf.read()
            sectors_needed = (len(data) + SECTOR_SIZE - 1) // SECTOR_SIZE
            cand = DATA_START_LBA
            while True:
                conflict = False
                for s in range(cand, cand + sectors_needed):
                    if s in used:
                        conflict = True
                        break
                if not conflict:
                    break
                cand += 1
                if cand > DATA_START_LBA + 100000:
                    raise Exception("can't find free data area")

            data_lba = cand
            write_file_data(fh, data_lba, data)

            new_file_idx = find_free(sp['file_bitmap'])
            if new_file_idx < 0: raise Exception("no file slots")
            new_file_lba = FILE_TABLE_START_LBA + new_file_idx
            fl = {'name': name, 'size': len(data), 'data_lba': data_lba}
            write_sector(fh, new_file_lba, build_file(fl))

            sp['file_bitmap'][new_file_idx] = 1
            write_sector(fh, SUPERBLOCK_LBA, build_super(sp))

            prog_dir['files_lba'][prog_dir['file_count']] = new_file_lba
            prog_dir['file_count'] += 1
            write_sector(fh, prog_dir_lba, build_dir(prog_dir))

            for s in range(data_lba, data_lba + sectors_needed):
                used.add(s)

            print("Injected", name, "-> file_table_lba", new_file_lba, "data_lba", data_lba, "size", len(data))

if __name__ == "__main__":
    main()