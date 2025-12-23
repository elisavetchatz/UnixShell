/*
 * parser.c - Command parsing and pipeline splitting
 */

#include "../include/parser.h"
#include <ctype.h>

// Parse a single command, checking for redirections (<, >, >>, 2>)
void parse_command(char *input, Command *cmd) 
{
    cmd->argc = 0;
    cmd->infile = NULL;
    cmd->outfile = NULL;
    cmd->errfile = NULL;
    cmd->append = 0;
    cmd->background = 0;
    
    // Check for & at the end (background execution)
    char *bg_pos = input;
    while (*bg_pos) bg_pos++;
    bg_pos--;
    
    // Skip trailing whitespace
    while (bg_pos > input && (*bg_pos == ' ' || *bg_pos == '\t' || *bg_pos == '\n')) 
        bg_pos--;
    
    // Check if last non-whitespace character is &
    if (bg_pos > input && *bg_pos == '&') 
    {
        cmd->background = 1;
        *bg_pos = '\0';  // Remove & from command string
    }
    
    char *cmd_start = input;
    char *pos = input;
    
    // Scan through input and extract all redirections
    while (*pos) 
    {
        if (pos[0] == '2' && pos[1] == '>') 
        {
            // Stderr redirection
            *pos = '\0';
            pos += 2;
            while (*pos == ' ' || *pos == '\t') pos++;
            
            char *start = pos;
            while (*pos && *pos != ' ' && *pos != '\t' && *pos != '<' && *pos != '>' && *pos != '\n') 
                pos++;
            
            if (pos > start) 
            {
                cmd->errfile = start;
                if (*pos) *pos++ = '\0';
            }
        }
        else if (pos[0] == '>' && pos[1] == '>') 
        {
            // Append redirection
            *pos = '\0';
            pos += 2;
            while (*pos == ' ' || *pos == '\t') pos++;
            
            char *start = pos;
            while (*pos && *pos != ' ' && *pos != '\t' && *pos != '<' && *pos != '>' && *pos != '\n') 
                pos++;
            
            if (pos > start) 
            {
                cmd->outfile = start;
                cmd->append = 1;
                if (*pos) *pos++ = '\0';
            }
        }
        else if (*pos == '>') 
        {
            // Output redirection
            *pos = '\0';
            pos++;
            while (*pos == ' ' || *pos == '\t') pos++;
            
            char *start = pos;
            while (*pos && *pos != ' ' && *pos != '\t' && *pos != '<' && *pos != '>' && *pos != '\n') 
                pos++;
            
            if (pos > start) 
            {
                cmd->outfile = start;
                cmd->append = 0;
                if (*pos) *pos++ = '\0';
            }
        }
        else if (*pos == '<') 
        {
            // Input redirection
            *pos = '\0';
            pos++;
            while (*pos == ' ' || *pos == '\t') pos++;
            
            char *start = pos;
            while (*pos && *pos != ' ' && *pos != '\t' && *pos != '<' && *pos != '>' && *pos != '\n') 
                pos++;
            
            if (pos > start) 
            {
                cmd->infile = start;
                if (*pos) *pos++ = '\0';
            }
        }
        else 
        {
            pos++;
        }
    }
    
    // Tokenize the command part (what's left after removing redirections)
    char *token = strtok(cmd_start, " \t\n");
    while (token && cmd->argc < MAX_ARGS - 1) 
    {
        if (*token) 
        {
            cmd->argv[cmd->argc++] = token;
        }
        token = strtok(NULL, " \t\n");
    }
    cmd->argv[cmd->argc] = NULL;
}

// Split input by | to get pipeline commands
int split_pipeline(char *input, Command cmds[]) 
{
    int num_cmds = 0;
    char *saveptr;
    char *cmd_str = strtok_r(input, "|", &saveptr);
    
    while (cmd_str && num_cmds < MAX_CMDS) 
    {
        // Skip leading whitespace
        while (*cmd_str && (*cmd_str == ' ' || *cmd_str == '\t')) 
            cmd_str++;
        
        if (*cmd_str) 
        {
            parse_command(cmd_str, &cmds[num_cmds]);
            if (cmds[num_cmds].argc > 0) 
                num_cmds++;
        }
        
        cmd_str = strtok_r(NULL, "|", &saveptr);
    }
    
    return num_cmds;
}
