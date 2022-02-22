# Filename: test_copy.py

# 将EventOS源码中的数据结构定义，拷贝到eos_test.c的特定位置去。
def execute():
   str_start = "// **eos**"
   str_end = "// **eos end**"

   # 
   f_eos = open("eventos/eventos.c", mode = 'r+', encoding = 'utf-8', newline = '\r\n')
   str_eos = f_eos.read()
   index_eos_start = str_eos.find(str_start)
   index_eos_end = str_eos.find(str_end)
   f_eos.seek(index_eos_start)
   str_def = f_eos.read(index_eos_end - index_eos_start)

   f_test = open("examples/test/eos_test.c", mode = 'r+', encoding = 'utf-8', newline = '\r\n')
   str_test = f_test.read()
   index_test_start = str_test.find(str_start)
   index_test_end = str_test.find(str_end)
   f_test.seek(0)
   str_head = f_test.read(index_test_start)
   f_test.seek(index_test_end)
   str_tail = f_test.read()
   #f_test.truncate(0)

   str_new_test = str_head
   str_new_test = str_new_test + str_def
   str_new_test = str_new_test + str_tail
   
   # f_test.write(str_new_test)
   # print(str_new_test)

   # print("eos start", index_eos_start, "end", index_eos_end)
   # print("test start", index_test_start, "end", index_test_end)

   f_eos.close()
   return