import os
import sys
import chardet

for filepath, dirnames, filenames in os.walk(r'/home/whm/Project/djysrc/loader'):
    for filename in filenames:
        filename = os.path.join(filepath, filename)
        print(filename)

        f = open(file = filename, mode = 'rb')
        data = f.read()
        dict_encode = chardet.detect(data)

        if dict_encode["encoding"] == 'GB2312' :
            print(dict_encode)
            f = open(filename, mode = 'r+', encoding = 'GB2312', newline = '\r\n')
            str = f.read()
            f.seek(0)
            f.truncate()
            f.close()

            str.encode('utf-8')

            f = open(filename, mode = 'w', encoding = 'utf-8', newline = '')
            f.write(str)
            f.close()

