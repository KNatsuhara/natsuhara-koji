#include <stdio.h>
#include <stdarg.h>


//-----------------------// PART 1 OF LAB1 //-----------------------//
typedef unsigned int u32;

char *tab = "0123456789ABCDEF";
int  BASE = 10; 

int rpu(u32 x)
{  
    char c;
    if (x){
       c = tab[x % BASE];
       rpu(x / BASE);
       putchar(c);
    }
}

int superRPU(u32 x, int base)
{  
    char c;
    if (x){
       c = tab[x % base];
       superRPU(x / base, base);
       putchar(c);
    }
}

int printu(u32 x)
{
   (x==0)? putchar('0') : rpu(x);
   putchar(' ');
}

int prints(char *s)
{
    char *ch = s;
    while(*ch != '\0')
    {
        putchar(*ch);
        ch++;
    }
}

int printd(int x) //prints out an integer (can be negative!)
{
    int positive;
    if (x < 0)
    {
        putchar('-');
        positive = x * -1;
        printu(positive);
    }
    else
    {
        printu(x);
    }
}

int printx(u32 x) //prints out HEX (start with 0x)
{
    putchar('0');
    putchar('x');
    (x==0)? putchar('0') : superRPU(x,16);
    putchar(' ');
}

int printo(u32 x) //prints out OCTAL (Start with 0)
{
    putchar('0');
    (x==0)? putchar('0') : superRPU(x,8);
    putchar(' ');
}

int myprintf(char *fmt, ...)
{
    char *cp = fmt; //Points at the fmt string
    int *ip = &fmt; //Set ip to the address of fmt
    ip++; //Increment pointer to point at the first item to printed in the stack
    while(*cp != '\0')
    {
        if(*cp == '%')
        {
            cp++;
            if(*cp == 'c')
            {
                (char)putchar(*ip); //Access all the arguments assigned to valist)
            }
            else if(*cp == 's')
            {
                prints(*ip);
            }
            else if(*cp == 'u')
            {
                printu(*ip);
            }
            else if(*cp == 'd')
            {
                printd(*ip);
            }
            else if(*cp == 'o')
            {
                printo(*ip);
            }
            else if(*cp == 'x')
            {
                printx(*ip);
            }
            ip++;
        }
        else
        {
            putchar(*cp);
        }
        cp++;
    }
}

int main(int argc, char *argv[], char *env[])
{
    //-----------------------// PART 1 OF LAB1 //-----------------------//
    for (int i = 0; i < argc; i++)
    {
        myprintf("argc[%d]=%s", i, argv[i]);
        printf("\n");
    }
    myprintf("cha=%c string=%s unsigned=%u signed=%d hex=%x oct=%o\n", 'A', "this is a test", 123, -123, 123, 123);
    printf("\n");
}