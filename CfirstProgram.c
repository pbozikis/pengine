#include<stdio.h> //necessary include for input/output (scanf and printf and more)

void toUpper(char * str); //this just tells the compiler we're using this function later

int main()
{
	int a;
	printf("Please enter a number: ");
	scanf("%d", &a); //scanf assigns var 'a' user input without returning the value like C# or python
	//instead the '&' means that the memory address of a is passed into the function and assigned that way
	//%d means that it should expect a decimal, %c a char etc
	printf("The variable %d is stored in memory address %x\n", a, &a); //%x is hexidecimal

	int * b; //This is an int pointer, it stores the memory address of a value
	b = &a; //b now points to a's memory address
	printf("value of what b points to: %d\n", *b); //the * dereferences the pointer, meaning you can get/set the actual value it points to

	//easy peezy squeeze the lemon
	//now let's CAPITALISE a string
	char string[20]; //Arrays are fixed size :(
	printf("Please enter a word: ");
	scanf("%s", string); //No & needed as arrays are pretty much a special type of pointer, pointing to the first item
	toUpper(string); //this function is void so it has no return value and we are altering the array
	printf("%s\n", string);
}

void toUpper(char * str)
{
	//for loop just like C# except C doesn't know how long our array is as we've only passed a pointer to it
	//we can explicitly tell it by passing it's length in but nah
	//C strings are ended by \0 so we looping until a \0
	for(int i = 0; str[i] != '\0'; i++)
	{
		//Lucky for us, C uses ascii where chars are just numbers and lowercase letters are 32 numbers higher than uppercase ones
		//so we just subtract 32 from each item?
		//yes, but only if the letter is lowercase, and not a number or symbol
		str[i] -= 32 * (str[i] >= 'a' && str[i] <= 'z');
		//The weird thing on the right is a branchless conditional, it replaces this:
		/*
		if(str[i] >= 'a' && str[i] <= 'z')
		{
			str[i] -= 32;
		}
		*/
		//the bracketed part either is true: 1 or false: 0
		//so it will cancel out the 32
	}
}
