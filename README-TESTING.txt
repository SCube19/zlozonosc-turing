Usage of the tester script:
- let DIR = your working directory
- put `tm_interpreter` binary in DIR
- put your binary in DIR and put its name in test.sh variable in the first line
- create DIR/tests directory and put your tests there
- all tests should consist of an input two-tape TM file (*.tm) and of input file (*.in). The input file has one input instance in each line.
- run the tester with simple `./test.sh`
