/* A small Brainf**k compiler using nasm.
It basically translates Brainf**k tokens to assembly
and then assembles the resulting code.

!! WARNING !! This is a Linux only compiler as it uses Linux syscalls (read, write, exit)
!! WARNING !! The compiler uses the file remove() command so it is recommended to run the compiler from it's directory
to avoid unwanted file deletion.

Specifications:
Brainf**k tape size: 30000 bytes */
#include <stdio.h>
#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <assert.h>

#define TOK_INC '>'
#define TOK_DEC '<'
#define TOK_ADD '+'
#define TOK_SUB '-'
#define TOK_PUT '.'
#define TOK_INP ','
#define TOK_OPN '['
#define TOK_CLS ']'

const bool optimizationsEnabled = true;


struct Token {
    char type;
    uint32_t arg0, arg1;
};


uint32_t findMatchingClose(char* code, size_t begin, size_t codeSize)
{
    uint32_t currentPos = begin + 1;
    uint32_t loopDepth = 1;
    while (loopDepth > 0)
    {
        if (currentPos >= codeSize)
        {
            printf("Syntax Error at character %li([): Unable to find a matching closing bracket(])\n",begin);
            throw std::runtime_error("Syntax Error"); 
        }

        char currChar = code[currentPos];
        if (currChar == '[') loopDepth++;
        else if (currChar == ']') loopDepth--;
        currentPos++;
    }
    return currentPos - 1;
}
uint32_t findMatchingOpen(char* code, size_t begin)
{
    uint32_t currentPos = begin - 1;
    uint32_t loopDepth = 1;
    while (loopDepth > 0)
    {
        char currChar = code[currentPos];
        if (currChar == '[') loopDepth--;
        else if (currChar == ']') loopDepth++;
        currentPos--;
 
        if (currentPos == UINT32_MAX) // should wrap around on decrement when 0
        {
            printf("Syntax Error at character %li(]): Unable to find a matching closing bracket([)\n", begin);
            throw std::runtime_error("Syntax Error"); 
        }
    }
    return currentPos + 1;
}


std::vector<Token> tokenize(char* code, size_t codeSize)
{
    std::vector<Token> tokens;
    for (size_t i = 0; i < codeSize; i++)
    {
        switch (code[i])
        {
        case TOK_INC:
            tokens.push_back(Token{TOK_INC, 1, 0});
            break;
        case TOK_DEC:
            tokens.push_back(Token{TOK_DEC, 1, 0});
            break;
        case TOK_ADD:
            tokens.push_back(Token{TOK_ADD, 1, 0});
            break;
        case TOK_SUB:
            tokens.push_back(Token{TOK_SUB, 1, 0});
            break;
        case TOK_PUT:
            tokens.push_back(Token{TOK_PUT, 0, 0});
            break;
        case TOK_INP:
            tokens.push_back(Token{TOK_INP, 0, 0});
            break;
        case TOK_OPN:
            tokens.push_back(Token{TOK_OPN, static_cast<uint32_t>(i), findMatchingClose(code, i, codeSize)});
            break;
        case TOK_CLS:
            tokens.push_back(Token{TOK_CLS, static_cast<uint32_t>(i), findMatchingOpen(code, i)});
            break;
        
        default:
            // ignore invalid tokens
            break;
        }
    }

    return tokens;    
}


void writeBegin(std::ofstream& stream)
{
    stream << "section .bss\n"
"    buffer_ptr: resb 30000\n"
"section .text\n"
"global _start\n"
"write:\n"
"    mov rax, 0x01\n"
"    mov rdi, 1\n"
"    mov rsi, r9\n"
"    mov rdx, 1\n"
"    syscall\n"
"    ret\n"
"read:\n"
"    mov rax, 0x00\n"
"    mov rdi, 0\n"
"    mov rsi, r9\n"
"    mov rdx, 1\n"
"    syscall\n"
"    ret\n"
"_start:\n"
"    mov r9, buffer_ptr\n";
}

void writeEnd(std::ofstream& stream)
{
    stream << "\n"
"    mov rax, 0x3C\n"
"    mov rdi, 0\n"
"    syscall\n";
}


void writeAsm(std::ofstream& stream, std::vector<Token>& toks)
{
    writeBegin(stream);

    for (auto& token : toks)
    {
        switch (token.type)
        {
        case TOK_INC:
            if (token.arg0 == 1)
                stream << "    inc r9\n";
            else if (token.arg0 > 1)
                stream << "    add r9, " << token.arg0 << "\n";
            break;
        case TOK_DEC:
            if (token.arg0 == 1)
                stream << "    dec r9\n";
            else if (token.arg0 > 1)
                stream << "    sub r9, " << token.arg0 << "\n";
            break;
        case TOK_ADD:
            if (token.arg0 == 1)
                stream << "    inc byte [r9]\n";
            else if (token.arg0 > 1)
                stream << "    add byte [r9], " << token.arg0 << "\n";
            break;
        case TOK_SUB:
            if (token.arg0 == 1)
                stream << "    dec byte [r9]\n";
            else if (token.arg0 > 1)
                stream << "    sub byte [r9], " << token.arg0 << "\n";
            break;
        case TOK_PUT:
            stream << "    call write\n";
            break;
        case TOK_INP:
            stream << "    call read\n";
            break;
        case TOK_OPN:
            // test isn't optimal here, because even though it takes up less space, it can't handle 2 memory locs and a mov costs more
            stream << "    cmp byte [r9], 0\n";
            stream << "    je jump_point_" << token.arg1 << "\n";
            stream << "jump_point_" << token.arg0 << ":\n";
            break;
        case TOK_CLS:
            stream << "    cmp byte [r9], 0\n";
            stream << "    jne jump_point_" << token.arg1 << "\n";
            stream << "jump_point_" << token.arg0 << ":\n";
            break;

        default:
            assert(false && "UNREACHABLE. Unknown token");
            break;
        }
    }

    writeEnd(stream);
}
std::vector<Token> optimizeRepeatingInstructions(std::vector<Token>& input)
{
    std::vector<Token> result;
    result.push_back(input[0]);

    Token& lastToken = input[0];
    uint32_t resultIdx = 0;
    for (size_t currTokIdx = 1; currTokIdx < input.size(); currTokIdx++)
    {
        Token& currentToken = input[currTokIdx];

        if (lastToken.type != currentToken.type || currentToken.type == TOK_OPN || 
            currentToken.type == TOK_CLS || currentToken.type == TOK_PUT || currentToken.type == TOK_INP)
        {
            result.push_back(currentToken);
            resultIdx++;
        }
        else
        {
            result[resultIdx] = Token{currentToken.type, result[resultIdx].arg0 + 1, 0};
        }
        
        lastToken = input[currTokIdx];
    }

    return result;
}

// this code is very bad todo: improve!
void execSystemCommands(const char* outname)
{
    std::string command = "nasm -f elf64 compilerout.asm -o compilerout.o && ld compilerout.o -o " + std::string(outname);
    if(system(command.c_str()) == 0)
    {
        if (remove("compilerout.asm") || remove("compilerout.o"))
        {
            std::cout << "Unable to remove compilerout.asm and/or compilerout.o\n";
        }
        else
        {
            std::cout << "Wrote executable file to " << outname << "\n";
        }
    }
    else
        std::cout << command<<"\n";

}


int main(int argc, char const *argv[])
{   
    if (argc < 3)
    {
        printf("Syntax: ./compiler <filename.bf> <binary>\n");
        return EXIT_FAILURE;
    }

    std::ifstream file(argv[1]);
    if(!file.is_open())
    {
        std::cerr << "Brainf**k file does not exist!\n";
        return EXIT_FAILURE;
    }

    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0);
    char* fileContents = new char[fileSize];
    file.read(&fileContents[0], fileSize);
    file.close();
    
    auto tokens = tokenize(fileContents, fileSize);

    std::ofstream outFile("compilerout.asm");

    if (optimizationsEnabled)
    {
        std::cout << "Optimization: Converting sequential inc/dec to add/sub\n";
        auto optimized = optimizeRepeatingInstructions(tokens);
        std::cout << "Optimization: Token Ratio (Original/Optimized) = " << tokens.size() << "/" << optimized.size() << "\n";
        writeAsm(outFile, optimized);
        outFile.close();
        std::cout << "Wrote file.\n";
        execSystemCommands(argv[2]);
        delete[] fileContents;
        return EXIT_SUCCESS;
    }


    writeAsm(outFile, tokens);
    outFile.close();
    std::cout << "Wrote file.\n";
    execSystemCommands(argv[2]);
    delete[] fileContents;

    return EXIT_SUCCESS;
}
