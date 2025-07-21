#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <fstream>
#include <cstdio>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <cmath>
#include "archivator/archivator.h"
namespace fs = std::filesystem;
class Terminal
{
public:
    Terminal()
    {
        tcgetattr(STDIN_FILENO, &orig_term);
    }
        
    void setRawMode()
    {
        termios raw = orig_term;
        raw.c_lflag &= ~(ECHO | ICANON);
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    }
    void restore()
    {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_term);
        std::system("clear");
    }
    static int getWindowHeigh()
    {
        winsize ws;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
        return ws.ws_row;
    }
private:
    termios orig_term;
};
class FileExplorer
{
public:
    FileExplorer() : cursor_pos(0), top_line(0) {
        current_path = fs::current_path();
        listDirectory();
    }
    void run()
    {
        Terminal term;
        term.setRawMode();
        char c;
        while ((c = getchar()) != 'q')
        {
            switch(c)
            {
                case 27: //Esc
                    if(getchar() == 91)
                    {
                        switch(getchar())
                        {
                            case 'A':
                                moveCursor(-1);
                            break;
                            case 'B':
                                moveCursor(1);
                            break;
                        }
                    }
                break;
                case '\n': 
                    openSelected();
                break;
                
                case 'h':
                    navigateToParent();
                break;
                
                case 'r':
                    refresh();
                break;
                case 'c': {
                    fs::path input_path = current_path / entries[cursor_pos];
                    if (!fs::is_directory(input_path)) {
                        fs::path output_path = input_path;
                        output_path += ".arc";
                        compress(input_path.string(), output_path.string());
                        refresh();
                    }
                    break;
                }
                case 'v': {
                    fs::path input_path = current_path / entries[cursor_pos];
                    if (!fs::is_directory(input_path)) {
                        if (input_path.extension() == ".arc") {
                            fs::path output_path = input_path;
                            output_path.replace_extension("");
                            decompress(input_path.string(), output_path.string());
                            refresh();
                        }
                    }
                    break;
                }
            }
            display();
        }
        term.restore();
    }
private:
    void listDirectory()
    {
        entries.clear();
        cursor_pos = 0;
        top_line = 0;
        if(current_path.has_parent_path())
        {
            entries.push_back("..");
        }
        for(const auto& entry : fs::directory_iterator(current_path))
        {
            entries.push_back(entry.path().filename());
        }
    }
    void display()
    {
        std::system("clear");
        
        std::cout << "╔════════════╦════════════════════════════════════════════════" << std::endl;
        std::cout << "║ BitArchive ║" << "you there: " << current_path << std::endl; 
        std::cout << "╚════════════╩════════════════════════════════════════════════" << std::endl;
        
        int max_line = Terminal::getWindowHeigh() - 5;
        int end_line = std::min(top_line + max_line, (int)entries.size());
        for(int i = top_line; i < end_line; i++)
        {
            if(i == cursor_pos)
            {
                std::cout << ">";
            }
            else
            {
                std::cout << " ";
            }
            fs::path full_path = current_path / entries[i];
            
            if(fs::is_directory(full_path))
            {
                std::cout << "[📁] ";
            }
            else
            {
                std::cout << "[📄] ";
            }
            std::cout << entries[i] << std::endl;
        }
        std::cout << "╔═════════╦══════════════╦═══════════════╦══════════╦════════════╗" << std::endl;
        std::cout << "║ q-выход ║ ↑↓-навигация ║ Enter-открыть ║ h-наверх ║ r-обновить ║" << std::endl;
        std::cout << "╚═════════╩══════════════╩═══════════════╩══════════╩════════════╝" << std::endl;
        std::cout << "╔═══════════════════════════════╦════════════════════════════════╗" << std::endl;
        std::cout << "║          C - Архивировать     ║      V - разархивировать      ║" << std::endl;
        std::cout << "╚═══════════════════════════════╩════════════════════════════════╝" << std::endl;
    }
    void moveCursor(int delta)
    {
        cursor_pos = std::max(0, std::min((int)entries.size() - 1, cursor_pos + delta));
        int max_lines = Terminal::getWindowHeigh() - 5;
        if(cursor_pos < top_line)
        {
            top_line = cursor_pos;
        }
        else if (cursor_pos >= top_line + max_lines)
        {
            top_line = cursor_pos - (max_lines + 1);
        }
    }
    void openSelected()
    {
        fs::path selected = entries[cursor_pos];
        fs::path new_path = current_path / selected;
        if(fs::is_directory(new_path))
        {
            current_path = new_path;
            listDirectory();
        }
        else
        {
            viewFile(new_path);
        }
    }
    void navigateToParent()
    {
        if(current_path.has_parent_path())
        {
            current_path = current_path.parent_path();
            listDirectory();
        }
    }
    void viewFile(const fs::path& path)
    {
        std::system("clear");
        std::cout << "File: " << path << std::endl;
        std::cout << "═══════════════════════════════════════════════" << std::endl;
        std::ifstream file(path);
        if(file)
        {
            std::string line;
            while(std::getline(file, line))
            {
                std::cout << line << std::endl;
            }
        }
        else
        {
            std::cout << "file oppening ERR";
        }
        std::cout << "Press eny key to return....";
        getchar();
    }
    void refresh()
    {
        listDirectory();
    }
    fs::path current_path;
    std::vector<fs::path> entries;
    int cursor_pos;
    int top_line;
};