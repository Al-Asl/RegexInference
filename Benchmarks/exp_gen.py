import random
import os
import re
import itertools
import numpy as np

def InfixesOf(word):
    ic = set()
    for i in range(len(word) + 1):
        for j in range(len(word) - i + 1):
            ic.add(word[j : i + j])
    return ic

def InfixClosureSizeIsOK(pos, neg, MaxSizeOfInfixClosure):
    ic = set()
    for p in pos:
        ic |= InfixesOf(p)
    for n in neg: 
        ic |= InfixesOf(n)
    if len(ic) > MaxSizeOfInfixClosure:
        return False
    return True

def GenerateTableOfWords(alphabet, maxLenOfWords):
    table = []
    for i in range(maxLenOfWords + 1):
        table.append([])
        table[i] = [''.join(list(w)) for w in list(itertools.product(alphabet, repeat = i))]
    return table

def GeneratePosNegSetsType1(posNum, negNum, lst):
    if posNum + negNum == 0:
        return [], []
    if posNum + negNum > len(lst):
        return [], []
    pn = list(np.random.choice(lst, posNum + negNum, replace = False))
    pos = np.random.choice(pn, size = posNum, replace = False)
    neg = np.setdiff1d(pn, pos);
    return list(pos), list(neg)

def extract_words(quoted_str):
    return re.findall(r'"(.*?)"', quoted_str)

def one_file_export(fileName, examples):
    f = open(fileName, "w")
    for exampleNum, (p, n) in enumerate(examples):
        f.write("Exp " + str(exampleNum + 1) + "\nP: " + p + "\nN: " + n + "\n\n")
    f.close()    

def multi_files_export(dirName, fileNamePrefix, fileHeaderPrefix, examples):
    output_dir = dirName
    os.makedirs(output_dir, exist_ok=True)

    for example_num, (pos, neg) in enumerate(examples, start=1):
        pos_words = extract_words(pos)
        neg_words = extract_words(neg)
        file_path = os.path.join(output_dir, f"{fileNamePrefix}_exp{example_num}.txt")
        with open(file_path, "w") as f:
            f.write(f"{fileHeaderPrefix}, Exp {example_num}\n")
            f.write("++\n")
            f.write('\n'.join(f"\"{word}\"" for word in pos_words))
            f.write("\n--\n")
            f.write('\n'.join(f"\"{word}\"" for word in neg_words))
    
def GenerateExamplesFileType1(seed, alphabet, minNumOfWords, maxNumOfWords, 
                         maxLenOfWords, lengthStride, repeatGeneration, MaxSizeOfInfixClosure):
    np.random.seed(seed)  
    examples = []
    if minNumOfWords == 0:
        examples.append(["", ""])
        minNumOfWords += 1
    for ln in range(0, maxLenOfWords + 1, lengthStride):
        lst = GenerateListOfWords(alphabet, ln)  
        for p in range(minNumOfWords, maxNumOfWords + 1):
            for n in range(minNumOfWords, maxNumOfWords + 1):
                for i in range(repeatGeneration):
                    pos, neg = GeneratePosNegSetsType1(p, n, lst)
                    if len(pos) + len(neg) > 0 and InfixClosureSizeIsOK(pos, neg, MaxSizeOfInfixClosure):  
                        examples.append(["\"" + "\" \"".join(pos) + "\"", "\"" + "\" \"".join(neg) + "\""])
    examples.sort(key = lambda x: len(x[0]) ** 2 + len(x[1]) ** 2)

    #one_file_export("Type1.txt", examples)
    multi_files_export("type1","type1","Type 1",examples)

    print(str(len(examples)) + " examples generated")
    
def GenerateListOfWords(alphabet, maxLenOfWords):
    lst = []
    for i in range(maxLenOfWords + 1):
        lst += [''.join(list(w)) for w in list(itertools.product(alphabet, repeat = i))]
    return lst

def GeneratePosNegSetsType2(posNum, negNum, table):
    if posNum + negNum == 0:
        return [], []
    if posNum + negNum > 2 ** (len(table)) - 1:
        return [], []
    pop = list(np.zeros(len(table), dtype = int))
    for i in range(posNum + negNum):
        rnd = np.random.randint(len(table))
        while pop[rnd] == len(table[rnd]):
            rnd = np.random.randint(len(table))
        pop[rnd] += 1
    words = []
    for i in range(len(table)):
        words += list(np.random.choice(table[i], pop[i], replace = False))
    pos = np.random.choice(words, size = posNum, replace = False)
    neg = np.setdiff1d(words, pos);
    return list(pos), list(neg)

def GenerateExamplesFileType2(seed, alphabet, minNumOfWords, maxNumOfWords, 
                         maxLenOfWords, lengthStride, repeatGeneration, MaxSizeOfInfixClosure):
    np.random.seed(seed)    
    examples = []
    if minNumOfWords == 0:
        examples.append(["", ""])
        minNumOfWords += 1
        
    for ln in range(0, maxLenOfWords + 1, lengthStride):
        table = GenerateTableOfWords(alphabet, ln)
        for p in range(minNumOfWords, maxNumOfWords + 1):
            for n in range(minNumOfWords, maxNumOfWords + 1):
                for i in range(repeatGeneration):
                    pos, neg = GeneratePosNegSetsType2(p, n, table)
                    if len(pos) + len(neg) > 0 and InfixClosureSizeIsOK(pos, neg, MaxSizeOfInfixClosure):  
                        examples.append(["\"" + "\" \"".join(pos) + "\"", "\"" + "\" \"".join(neg) + "\""])
    examples.sort(key = lambda x: len(x[0]) ** 2 + len(x[1]) ** 2)

    #one_file_export("Type2.txt", examples)
    multi_files_export("type2","type2","Type 2",examples)

    print(str(len(examples)) + " examples generated")

def GenerateType1():
    seed = 0
    alphabet = ['0', '1']
    minNumOfWords = 16            # from (#pos, #neg) = (n, n)
    maxNumOfWords = 128           # to   (#pos, #neg) = (m, m)
    maxLenOfWords = 7             # words up to len k
    lengthStride = 7              # to decrease the number of examples
    repeatGeneration = 1          # to increase the number of examples
    MaxSizeOfInfixClosure = 126   # Len(IC(.)) <= 126

    GenerateExamplesFileType1(seed, alphabet, minNumOfWords, maxNumOfWords,
                        maxLenOfWords, lengthStride, repeatGeneration, MaxSizeOfInfixClosure)
    
def GenerateType2():
    seed = 0
    alphabet = ['0', '1']
    minNumOfWords = 16            # from (#pos, #neg) = (n, n)
    maxNumOfWords = 128           # to   (#pos, #neg) = (m, m)
    maxLenOfWords = 10            # words up to len k
    lengthStride = 8              # to decrease the number of examples
    repeatGeneration = 1          # to increase the number of examples
    MaxSizeOfInfixClosure = 126   # Len(IC(.)) <= 126

    GenerateExamplesFileType2(seed, alphabet, minNumOfWords, maxNumOfWords,
                        maxLenOfWords, lengthStride, repeatGeneration, MaxSizeOfInfixClosure)
    
GenerateType1()
GenerateType2()