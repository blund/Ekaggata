
inn = open("test.lasm", "r")
ut  = open("build/test.xlasm", "w+")
lines = inn.readlines()

labels = {}


labels["test"] = 2

print(labels)




# I første 'pass' vil vi bare fjerne tomme linjer og
# erstatte labels med linjetall
end = len(lines)
i = 0
while(i < end):

    # Rydd bort tomme linjer
    if lines[i] == '\n':
        lines = lines[:i] + lines[i+1:]
        print("BOM!")
        end = len(lines)
        continue
    
    has_colon = lines[i].find(":")

    if(has_colon != -1):
        label = lines[i][:-2]
        labels[label] = i+1 # Legg til i labels (vil peke på neste linje), fjern semikolon
        lines = lines[:i] + lines[i+1:]
        print("BOM!")
        end = len(lines)
        continue
        
    # print(has_colon)
    
    i+=1

# I andre 'pass' her vil vi erstatte labels med tilsvarende linje
# Her setter vi også inn X-macro-markeringen
    
i = 0
while(i < end):
    if lines[i][0] == 'J':
        for keys, val in labels.items():
            lines[i] = lines[i].replace(keys, str(val))

    lines[i] = lines[i][:lines[i].find("//")] + " "
    lines[i].replace('\n', '')
    
    if (i == end-1):
        lines[i] = ("X(%s) \n" % lines[i])
    else:    
        lines[i] = ("X(%s) \\\n" % lines[i][0:-1])

    i+=1


# Og til slutt outputter vi greier..
for line in lines:
    print(line)
    ut.write(line)
