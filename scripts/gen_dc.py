import os
import re
import random
import csv
import re
import rstr

def generate_matching_words(regex, num_words, min_length, max_length, pre_words = [], attempt_per_word = 1000):

    words = set(pre_words)

    attempts = 0
    max_attempts = num_words * attempt_per_word

    while len(words) < num_words and attempts < max_attempts:
        candidate = rstr.xeger(regex)
        if min_length <= len(candidate) <= max_length:
            words.add(candidate)
        attempts += 1

    return list(words)

def find_alphabet(regex):
    alphabet = { char for char in regex if not char in ['(',')','*','?','|'] }
    return list(alphabet)

def generate_non_matching_words(regex, num_words, min_length, max_length, pre_words = [], attempt_per_word = 1000):

    alphabet = find_alphabet(regex)
    pattern = re.compile(regex)
    words = set(pre_words)

    attempts = 0
    max_attempts = num_words * attempt_per_word

    while len(words) < num_words and attempts < max_attempts:
        length = random.randint(min_length, max_length)
        word = ''.join(random.choices(alphabet, k=length))
        if not pattern.fullmatch(word):
            words.add(word)
        attempts += 1

    return list(words)

def read_benchmarks(file):

    res = []

    with open(file, 'r', newline='') as file:
        reader = csv.reader(file)
        next(reader)
        for row in reader:
            res.append({
                "index": row[0],
                "b_type": row[1],
                "re": row[7]
            })
    return res

def read_benchmark_file(file_path):
    pos = []
    neg = []

    with open(file_path, 'r', newline='') as file:
        next(file) 
        next(file) 
        read_pos = True
        for line in file:
            if line.startswith("--"):
                read_pos = False
                continue

            match = re.search(r'"([\d\w]+)"', line)
            reg = match.group(1) if match else ""

            if read_pos:
                pos.append(reg)
            else:
                neg.append(reg)
    
    return (pos,neg)

def file_export(file_path, fileHeader, examples):

    (pos_words, neg_words) = examples

    with open(file_path, "w") as f:
        f.write(fileHeader + "\n")
        f.write("++\n")
        f.write('\n'.join(f"\"{word}\"" for word in pos_words))
        f.write("\n--\n")
        f.write('\n'.join(f"\"{word}\"" for word in neg_words))

def generate(num_words, min_length, max_length):

    benchmarks = read_benchmarks('benchmarks.csv')

    for index, benchmark in enumerate(benchmarks):

        file_path = f"./type{benchmark['b_type']}/type{benchmark['b_type']}_exp{benchmark['index']}.txt"

        examples = read_benchmark_file(file_path)
        pos = generate_matching_words(benchmark['re'], num_words/2, min_length, max_length, examples[0])
        neg = generate_non_matching_words(benchmark['re'], num_words/2, min_length, max_length, examples[1])

        print(f"Index {index + 1}: number of pos {len(pos)}, number of neg {len(neg)}")

        file_export(f"./dc/dc_exp{index + 1}.txt",f"DC, EXP {index + 1}", (pos,neg))

generate(128, 1, 14)