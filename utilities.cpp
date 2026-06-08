# include "splashkit.h"

string read_string(string prompt){

    write(prompt);
    return read_line();
}

int read_int(string prompt){
    string input;

    input = read_string(prompt);

    while (!is_integer(input))
    {
        write_line("Please enter an integer!");
        input = read_string(prompt);
    }

    return to_integer(input);
}

int read_int(string prompt, int min, int max){
    int input;
    input = read_int(prompt);

    while(input < min || input > max)
    {
        write_line("Please enter an integer between " + to_string(min) + " and " + to_string(max));
        input = read_int(prompt);
    }

    return input;
}

double read_double(string prompt){
    string input;

    input = read_string(prompt);

    while (!is_double(input))
    {
        write_line("Please enter a number!");
        input = read_string(prompt);
    }

    return to_double(input);
}