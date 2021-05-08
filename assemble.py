
print(' [ PYTHON KOMPILER ] \nKompilerer "test.lasm" til "build/test.xlasm"')

inn = open("test.lasm", "r")
ut  = open("build/test.xlasm", "w+")

lines = inn.readlines()


while True:
    if lines[-1] == "\n" or lines[-1] == "":
        lines = lines[:-1]
    else:
        break

        
for i in range(len(lines)):
    lines[i] = lines[i][:lines[i].find("//")] + " "
    lines[i].replace('\n', '')
    
    if lines[i] == ' ':
        continue
    
    if (lines[i] == lines[-1]):
        ut.write("X(%s) \n" % lines[i])
    else:    
        ut.write("X(%s) \\\n" % lines[i][0:-1])

        
