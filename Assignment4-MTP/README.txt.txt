Author: Long To Lotto Tang

OSU CS344 - MTP

Descriptions:

- familiar with the use of threads, mutual exclusion, condition variables and producer-consumer aapproach

- Thread 1, called the Input Thread, reads in lines of characters from the standard input.
- Thread 2, called the Line Separator Thread, replaces every line separator in the input by a space.
- Thread, 3 called the Plus Sign thread, replaces every pair of plus signs, i.e., "++", by a "^".
- Thread 4, called the Output Thread, write this processed data to standard output as lines of exactly 80 characters.


Instructions:

1) Copy line_processor.c into your server

2) Compile using the following command in unix:

    gcc -pthread -std=c99 -o line_processor line_processor.c 

3) Type the following command to run on your server:
    
    ./line_processor

4) To redirect the input and output (e.g. input comes from input1.txt, output goes to output1.txt), type the following:

    ./line_processor < input1.txt > output1.txt