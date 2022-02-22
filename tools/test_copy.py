# Filename: test_copy.py

# 将EventOS源码中的数据结构定义，拷贝到eos_test.c的特定位置去。
def execute():
   str_start = "// **eos**"
   str_end = "// **eos end**"
   path_eos = "eventos/eventos.c"
   path_test = "examples/test/eos_test.c"

   # 将EventOS数据结构定义部分拷贝出来
   f_eos = open(path_eos, mode = 'r+', encoding = 'utf-8', newline = '\r\n')
   str_eos = f_eos.read()
   index_eos_start = str_eos.find(str_start)
   index_eos_end = str_eos.find(str_end)
   f_eos.seek(index_eos_start)
   str_def = f_eos.read(index_eos_end - index_eos_start)
   f_eos.close()

   # 将Test的头尾拷贝出来，清空文件
   f_test = open(path_test, mode = 'r+', encoding = 'utf-8', newline = '\r\n')
   str_test = f_test.read()
   index_test_start = str_test.find(str_start)
   index_test_end = str_test.find(str_end)
   f_test.seek(0)
   str_head = f_test.read(index_test_start)
   f_test.seek(index_test_end)
   str_tail = f_test.read()
   f_test.seek(0)
   f_test.truncate()
   f_test.close()

   # 拼接为一个完整文件，并写入进去
   str_new_test = str_head
   str_new_test = str_new_test + str_def
   str_new_test = str_new_test + str_tail
   f_test = open(path_test, mode = 'w', encoding = 'utf-8', newline = '')
   f_test.write(str_new_test)
   f_test.close()

   return