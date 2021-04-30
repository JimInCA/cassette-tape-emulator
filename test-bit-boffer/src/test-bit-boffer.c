/*
 * Copyright(c) 2021 - Jim Newman
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to the following
 * conditions:

 * The above copyright notice and this permission notice shall be included in all copies
 * or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <Windows.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>

unsigned long verbose = 0;

HANDLE trn_uart;
HANDLE rcv_uart;

int16_t init_uart(HANDLE *uart, uint8_t *serial_port, uint32_t baud, uint32_t stopbits)
{
    COMMTIMEOUTS timeouts = { 0 };

    timeouts.ReadIntervalTimeout = 20;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    timeouts.ReadTotalTimeoutConstant = 100;
    timeouts.WriteTotalTimeoutMultiplier = 10;
    timeouts.WriteTotalTimeoutConstant = 100;

    uint8_t adjusted_serial_port[16];

    sprintf(adjusted_serial_port, "\\\\.\\%s", serial_port);
    *uart = CreateFile(adjusted_serial_port,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (*uart == INVALID_HANDLE_VALUE)
    {
        printf("Failed to connet to UART on port %s\n", serial_port);
        return 1;
    }
    else
    {
        printf("Successfully connected to UART on port %s at baud rate %d.\n", serial_port, baud);
    }

    DCB dcb = { 0 };

    dcb.DCBlength = sizeof(dcb);

    BOOL ret = GetCommState(*uart, &dcb);

    if (ret == FALSE)
    {
        ret = GetLastError();
        printf("Error getting current DCB settings: %d\n", ret);
    }
    //else
    //    printf("DCB is ready for use.\n");

    FillMemory(&dcb, sizeof(dcb), 0);
    if (!GetCommState(*uart, &dcb))     // get current DCB
    {
        printf("Error in GetCommState.\n");
        return 1;
    }

    // Update DCB rate.
    dcb.BaudRate = baud;
    dcb.StopBits = stopbits;

    // Set new state.
    if (!SetCommState(*uart, &dcb))
    {
        printf("Error in SetCommState. Possibly a problem with the communications,\n");
        printf("port handle, or a problem with the DCB structure itself.\n");
        return 1;
    }

    if (!SetCommTimeouts(*uart, &timeouts))
        printf("Error setting time-outs.\n");

    return 0;
}

uint8_t char_2_uint8(uint8_t ch)
{
    if ((ch >= '0') && (ch <= '9'))
        return (ch - '0');
    else if ((ch >= 'A') && (ch <= 'F'))
        return (ch - 'A' + 0x0a);
    else if ((ch >= 'a') && (ch <= 'f'))
        return (ch - 'a' + 0x0a);
    else
        printf("Error in character conversion.\n");
    return 0;
}

uint8_t get_byte(uint8_t *data, int16_t length)
{
    int16_t i, j;
    uint8_t temp = 0;
    i = 0;
    j = length * 2;
    do
    {
        //printf("%c ", data[i]);
        temp *= 16;
        temp += char_2_uint8(data[i++]);
        //printf(" 0x%02x\n", temp);
    } while (i < j);
    return temp;
}

void send_packet(HANDLE uart, uint8_t *packet, int16_t length)
{
    int16_t i;
    unsigned long wr_len;

    //write(uart, packet, length);  // JTN
    WriteFile(uart, packet, length, &wr_len, NULL);
    if (verbose > 1)
    {
        for (i = 0; i < length; i++) {
            printf("sending 0x%x ", packet[i]);
            if (i % 16 == 15)
                printf("\n");
        }
        printf("\n");
    }
}


int main(int argc, char **argv)
{
    int16_t error = 0;

    char *trn_portname = NULL;
    char *rcv_portname = NULL;

    unsigned long baudrate = 300;
    unsigned long help = 0;
    unsigned long test_num = 0;
    unsigned long num_test_loop = 128;

    int16_t i;
    long unsigned int num_read;
    uint8_t out_char, in_char;
    uint8_t in_buff[16];

    // simple command line parser
    for (i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-i") == 0)
        {
            trn_portname = argv[++i];
        }
        else if (strcmp(argv[i], "-o") == 0)
        {
            rcv_portname = argv[++i];
        }
        else if (strcmp(argv[i], "-b") == 0)
        {
            baudrate = atol(argv[++i]);
        }
        else if (strcmp(argv[i], "-t") == 0)
        {
            test_num = atol(argv[++i]);
        }
        else if (strcmp(argv[i], "-n") == 0)
        {
            num_test_loop = atol(argv[++i]);
        }
        else if (strcmp(argv[i], "-v") == 0)
        {
            verbose = atol(argv[++i]);
        }
        else if (strcmp(argv[i], "-h") == 0)
        {
            help = 1;
        }
        else
        {
            printf("Unknown input paramter: %s\n", argv[i]);
            return -1;
        }
    }

    if ((help == 1) || (trn_portname == NULL) || ((rcv_portname == NULL) && (test_num > 0)))
    {
        int result = 0;
        printf("usage: test-bit-boffer [-h] -i COMPORT [-o COMPORT] [-b BAUDRATE] [-t TESTNUM] [-n LOOPNUM]\n\n");
        if (help == 1)
        {
            printf("arguments:\n");
            printf("  -h             Show this help message and exit.\n");
            printf("  -i COMPORT     Transmitter COM Port.\n");
            printf("  -o COMPORT     Receiver COM Port when TESTNUM > 0.\n");
            printf("  -b BAUDRATE    Desired baudrate, default: 300.\n");
            printf("  -t TESTNUM     Desired test to be executed.\n");
            printf("  -n LOOPNUM     Number of test cycles in loop.\n");
        }
        else
        {
            if (trn_portname == NULL)
                printf("test-bit-boffer error: argument -i COMPORT is required.\n");
            result = -1;
        }
        return result;
    }

    if (init_uart(&trn_uart, trn_portname, baudrate, TWOSTOPBITS) != 0)
    {
        printf("Error opening %s: %s\n", trn_portname, strerror(errno));
        return -1;
    }

    printf("Connected to transmitter port %s at baudrate %ld\n", trn_portname, baudrate);

    if ((rcv_portname != NULL) && (test_num > 0))
    {
        if (init_uart(&rcv_uart, rcv_portname, baudrate, TWOSTOPBITS) != 0)
        {
            printf("Error opening %s: %s\n", rcv_portname, strerror(errno));
            return -1;
        }

        printf("Connected to receiver port %s at baudrate %ld\n", rcv_portname, baudrate);
    }

    fflush(stdout);

    // let's see if we can flush any waiting bytes from the uart buffer before we begin
    do
    {
        ReadFile(rcv_uart, in_buff, sizeof(in_buff), &num_read, NULL);
    } while (num_read != 0);

    switch (test_num)
    {
    case 0:  for (i = 0; i < 0x100; i++)
             {
                 out_char = (uint8_t)i;
                 send_packet(trn_uart, &out_char, 1);
             }
             break;
    case 1:  for (i = 0; i < 0x100; i++)
             {
                 out_char = (uint8_t)i;
                 send_packet(trn_uart, &out_char, 1);
                 ReadFile(rcv_uart, in_buff, sizeof(in_buff), &num_read, NULL);
                 if (verbose > 0)
                 {
                     in_buff[num_read] = '\0';
                     printf("sent 0x%02x -> received %s", out_char, in_buff);
                     fflush(stdout);
                 }
                 if ((in_buff[0] == '0') && (in_buff[1] == 'x'))
                 {
                     in_char = get_byte(&in_buff[2], 1);
                     if (out_char != in_char)
                     {
                         error -= 1;
                         printf("Error: sent 0x%02x -> received 0x%02x\n", out_char, in_char);
                         fflush(stdout);
                     }
                 }
             }
             break;
    case 2:
    case 4:  {
                 out_char = 0xff;
                 for (i = 0; i < num_test_loop; i++)
                 {
                     send_packet(trn_uart, &out_char, 1);
                     ReadFile(rcv_uart, in_buff, sizeof(in_buff), &num_read, NULL);
                     if (verbose > 0)
                     {
                         in_buff[num_read] = '\0';
                         printf("sent 0x%02x -> received %s", out_char, in_buff);
                         fflush(stdout);
                     }
                     if ((in_buff[0] == '0') && (in_buff[1] == 'x'))
                     {
                         in_char = get_byte(&in_buff[2], 1);
                         if (out_char != in_char)
                         {
                             error -= 1;
                             printf("Error: sent 0x%02x -> received 0x%02x\n", out_char, in_char);
                             fflush(stdout);
                         }
                     }
                 }
             }
             if (test_num == 2) 
                 break;
    case 3:  {
                 uint8_t out_data[] = {0x53, 0x00, 0x00, 0x00, 0x25, 0x86, 0x3e, 0xb7,
                                       0xe4, 0x1d, 0x86, 0x6d, 0xb7, 0xe4, 0x1e, 0x86,
                                       0x79, 0xb7, 0xe4, 0x1f, 0x86, 0x00, 0xb7, 0xe4,
                                       0x20, 0x86, 0x5e, 0xb7, 0xe4, 0x21, 0x86, 0x6d,
                                       0xb7, 0xe4, 0x22, 0x86, 0xa2, 0xff, 0xe4, 0x19,
                                       0x7e, 0xf0, 0xbb, 0xe4                          };
                 for (i = 0; i < sizeof(out_data); i++)
                 {
                     out_char = out_data[i];
                     send_packet(trn_uart, &out_char, 1);
                     ReadFile(rcv_uart, in_buff, sizeof(in_buff), &num_read, NULL);
                     if (verbose > 0)
                     {
                         in_buff[num_read] = '\0';
                         printf("sent 0x%02x -> received %s", out_char, in_buff);
                         fflush(stdout);
                     }
                     if ((in_buff[0] == '0') && (in_buff[1] == 'x'))
                     {
                         in_char = get_byte(&in_buff[2], 1);
                         if (out_char != in_char)
                         {
                             error -= 1;
                             printf("Error: sent 0x%02x -> received 0x%02x\n", out_char, in_char);
                             fflush(stdout);
                         }
                     }
                 }
             }
             break;
    case 5:  {
                 srand(time(0));  // Initialize random number generator.
                 for (i = 0; i < num_test_loop; i++)
                 {
                     out_char = rand();
                     send_packet(trn_uart, &out_char, 1);
                     ReadFile(rcv_uart, in_buff, sizeof(in_buff), &num_read, NULL);
                     if (verbose > 0)
                     {
                         in_buff[num_read] = '\0';
                         printf("sent 0x%02x -> received %s", out_char, in_buff);
                         fflush(stdout);
                     }
                     if ((in_buff[0] == '0') && (in_buff[1] == 'x'))
                     {
                         in_char = get_byte(&in_buff[2], 1);
                         if (out_char != in_char)
                         {
                             error -= 1;
                             printf("Error: sent 0x%02x -> received 0x%02x\n", out_char, in_char);
                             fflush(stdout);
                         }
                     }
                 }
             }
             break;
    default: printf("Invalid test number %d\n", test_num);
             return -1;
             break;
    }

    printf("Test %d %s!\n", test_num, ((error == 0) ? "Passed":"Failed"));

    // clean up
    CloseHandle(trn_uart);
    CloseHandle(rcv_uart);

    return 0;
}

