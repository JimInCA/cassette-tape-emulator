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

unsigned long verbose = 0;

HANDLE uart;

typedef struct _S_RECORD {
        uint8_t    type;
        uint16_t   length;
        uint16_t   start_addr;
        uint8_t  * data;
        uint16_t   checksum;
} S_RECORD;

typedef struct _KCS {
        uint8_t    mark;
        uint8_t    start_addr_high_byte;
        uint8_t    start_addr_low_byte;
        uint8_t    stop_addr_high_byte;
        uint8_t    stop_addr_low_byte;
        uint16_t   length;
        uint8_t  * data;
        uint8_t    checksum;
} KCS;


int16_t init_uart(uint8_t *serial_port, uint32_t baud, uint32_t stopbits)
{
    COMMTIMEOUTS timeouts = { 0 };

    timeouts.ReadIntervalTimeout = 20;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    timeouts.ReadTotalTimeoutConstant = 100;
    timeouts.WriteTotalTimeoutMultiplier = 10;
    timeouts.WriteTotalTimeoutConstant = 100;

    uint8_t adjusted_serial_port[16];

    sprintf(adjusted_serial_port, "\\\\.\\%s", serial_port);
    uart = CreateFile(adjusted_serial_port,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (uart == INVALID_HANDLE_VALUE)
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

    BOOL ret = GetCommState(uart, &dcb);

    if (ret == FALSE)
    {
        ret = GetLastError();
        printf("Error getting current DCB settings: %d\n", ret);
    }
    else
        printf("DCB is ready for use.\n");

    FillMemory(&dcb, sizeof(dcb), 0);
    if (!GetCommState(uart, &dcb))     // get current DCB
    {
        printf("Error in GetCommState.\n");
        return 1;
    }

    // Update DCB rate.
    dcb.BaudRate = baud;
    dcb.StopBits = stopbits;

    // Set new state.
    if (!SetCommState(uart, &dcb))
    {
        printf("Error in SetCommState. Possibly a problem with the communications,\n");
        printf("port handle, or a problem with the DCB structure itself.\n");
        return 1;
    }

    if (!SetCommTimeouts(uart, &timeouts))
        printf("Error setting time-outs.\n");

    return 0;
}

void send_packet(HANDLE uart, uint8_t *packet, int16_t length)
{
    int16_t i;
    unsigned long wr_len;

    //write(uart, packet, length);  // JTN
    WriteFile(uart, packet, length, &wr_len, NULL);
    if (verbose > 0)
    {
        printf("Sending Packet: %d\n", length);
        for (i = 0; i < length; i++) {
            printf("0x%x ", packet[i]);
            if (i % 16 == 15)
                printf("\n");
        }
        printf("\n");
    }
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


int main(int argc, char **argv)
{
    char *portname = NULL;
    char *file_name = NULL;

    unsigned long baudrate = 300;
    unsigned long flowcontrol = 0;
    unsigned long help = 0;
    unsigned long header_char = 819;

    FILE *fp;
    int16_t i, j, k;
    char in_str[128];
    S_RECORD *s_record;
    int16_t num_s_record_strings;
    KCS kcs;
    uint8_t checksum;

    // simple command line parser
    for (i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-f") == 0)
        {
            file_name = argv[++i];
        }
        else if (strcmp(argv[i], "-p") == 0)
        {
            portname = argv[++i];
        }
        else if (strcmp(argv[i], "-b") == 0)
        {
            baudrate = atol(argv[++i]);
        }
        else if (strcmp(argv[i], "-v") == 0)
        {
            verbose = atol(argv[++i]);
        }
        else if (strcmp(argv[i], "-c") == 0)
        {
            header_char = atol(argv[++i]);
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

    if ((help == 1) || (file_name == NULL) || (portname == NULL))
    {
        int result = 0;
        printf("usage: bit-boffer-writer [-h] -f FILE -p COMPORT [-b BAUDRATE]\n\n");
        if (help == 1)
        {
            printf("arguments:\n");
            printf("  -h             Show this help message and exit.\n");
            printf("  -f FILE        Filename of S-Record input file.\n");
            printf("  -p COMPORT     COM Port to which the device is connected.\n");
            printf("  -b BAUDRATE    Desired baudrate, default: 300.\n");
        }
        else
        {
            if (file_name == NULL)
                printf("hci_dfu_send_hex: error: argument -f FILE is required.\n");
            if (portname == NULL)
                printf("hci_dfu_send_hex: error: argument -p COMPORT is required.\n");
            result = -1;
        }
        return result;
    }

    if (init_uart(portname, baudrate, TWOSTOPBITS) != 0)
    {
        printf("Error opening %s: %s\n", portname, strerror(errno));
        return -1;
    }

    printf("Sending file %s to port %s at baudrate %ld\n", file_name, portname, baudrate);
    fflush(stdout);

    // let's open the file and check for errors
    fp = fopen(file_name, "r");
    if (fp == NULL)
    {
        printf("Error while opening file %s.\n", file_name);
        return -1;
    }

    // The first thing that I need to know is the number of S records 
    // are in the file.  This means that I will need to read the string 
    // data from the file at least twice.
    num_s_record_strings = 0;
    while ( fgets(in_str, 128, fp) != NULL )
    {
        num_s_record_strings++;
    }
    rewind(fp);
    //printf("num_s_record_strings: %d\n", num_s_record_strings);

    // next, we want to allocate the memory for an array of s records
    s_record = malloc(num_s_record_strings);

    // time to parse the input file
    i = 0;
    while ( fgets(in_str, 128, fp) != NULL )
    {
        //printf("%s", in_str);
        checksum = 0;
        // the first character in an s-record will be 'S'
        if ( (in_str[0] == 'S') && ((in_str[1] == '1') || (in_str[1] == '9')) )
        {
            //printf("S%c\n", in_str[1]);
            // Type
            s_record[i].type = (in_str[1] == '1') ? 1:9;
            //printf("type %d\n", s_record[i].type);
            // Length
            s_record[i].length = get_byte(&in_str[2], 1);
            checksum += (uint8_t)((s_record[i].length >> 8) & 0xff);
            checksum += (uint8_t)((s_record[i].length >> 0) & 0xff);
            //printf("length 0x%02x\n", s_record[i].length);
            // Address
            s_record[i].start_addr =  get_byte(&in_str[4], 2);
            checksum += s_record[i].start_addr & 0xff;
            checksum += (s_record[i].start_addr & 0xff00) > 8;
            //printf("address 0x%04x\n", s_record[i].start_addr);
            // Data
            s_record[i].data = malloc(s_record[i].length*2);
            if (s_record[i].data == NULL)
            {
                printf("Error while allocate memory for s-record.\n");
                return -1;
            }

            for (j = 8, k = 0; k < s_record[i].length-3; j+=2, k++)
            {
                s_record[i].data[k] = get_byte(&in_str[j], 1);
                //printf("data[%d] 0x%02x\n", k, s_record[i].data[k]);
                checksum += s_record[i].data[k];
            }
            // Checksum
            s_record[i].checksum =  (~get_byte(&in_str[j], 1) & 0x00ff);
            //printf("checksum 0x%02x\n", s_record[i].checksum);
            //printf("my checksum 0x%02x\n", checksum);
            if (s_record[i].checksum != checksum)
            {
                printf("Error in s-record.  Checksum Error. \n");
                return -1;
            }
        }
        else
        {
            printf("Invalid S-Record string: %s", in_str);
            return -1;
        }
        i++;
    }

    // Now that I have an array of s-records, I need to transfer the information 
    // and data to the output structure diffined for Kanas City Standard
    // Let's first go through the s-record array and calculate the total
    // length of the outout data.
    kcs.mark = 'S';
    kcs.length = 0;
    for (i = 0; i < num_s_record_strings; i++)
    {
        kcs.length += s_record[i].length-3; // remove s-record type and length
    }
    //printf("kcs.length: %d\n", kcs.length);
    // now that we know the size, let's allocate memory to hold output data
    kcs.data = malloc(kcs.length);
    if (kcs.data == NULL)
    {
        printf("Error while allocating memory for kcs structure.\n");
        return -1;
    }
    // now copy data from s-records to kcs
    for (i = 0, j = 0, kcs.checksum = 0; i < num_s_record_strings; i++)
    {
        switch (s_record[i].type)
        {
        case 1:
            for (k = 0; k < s_record[i].length-3; k++)
            {
                kcs.data[j++] = s_record[i].data[k];
                kcs.checksum += s_record[i].data[k];
            }
            break;
        case 9:
            kcs.start_addr_high_byte = (s_record[i].start_addr >> 8) & 0x00ff;
            kcs.start_addr_low_byte  = (s_record[i].start_addr >> 0) & 0x00ff;
            int16_t end_addr = s_record[i].start_addr + kcs.length - 1;
            kcs.stop_addr_high_byte = ((end_addr >> 8) & 0x00ff);
            kcs.stop_addr_low_byte  = ((end_addr >> 0) & 0x00ff);
            kcs.checksum += kcs.start_addr_high_byte;
            kcs.checksum += kcs.start_addr_low_byte;
            kcs.checksum += kcs.stop_addr_high_byte;
            kcs.checksum += kcs.stop_addr_low_byte;
            kcs.checksum = ~kcs.checksum + 1;  // 2's complement checksum
            break;
        }
    }

    if (verbose > 0)
    {
        printf("kcs.mark %c\n", kcs.mark);
        printf("kcs.start_addr_high_byte 0x%02x\n", kcs.start_addr_high_byte);
        printf("kcs.start_addr_low_byte 0x%02x\n", kcs.start_addr_low_byte);
        printf("kcs.stop_addr_high_byte 0x%02x\n", kcs.stop_addr_high_byte);
        printf("kcs.stop_addr_low_byte 0x%02x\n", kcs.stop_addr_low_byte);
        printf("kcs.length 0x%02x\n", kcs.length);
        for (i = 0, checksum = 0; i < kcs.length; i++)
        {
            printf("kcs.data[0x%02x] 0x%02x\n", i, kcs.data[i]);
        }
        printf("kcs.checksum 0x%02x\n", kcs.checksum);
    }

    // Now it's time to write the file to the uart in Kanas City Standard
    // format.
    // the first thing to do is wite the 30 second lead of 0xff
    // at 300 baud, 1 start bit, 8 data bits, 2 stop bits 
    // 30 * 300 = 9000, 9000 / 11 = 818.181 bytes
    // rounded up to 819
    uint8_t out_byte = 0xff;
    for (i = 0; i < header_char; i++)
    {
        send_packet(uart, &out_byte, 1);
    }
    // now send the rest of the data
    send_packet(uart, &kcs.mark, 1);
    send_packet(uart, &kcs.start_addr_high_byte, 1);
    send_packet(uart, &kcs.start_addr_low_byte, 1);
    send_packet(uart, &kcs.stop_addr_high_byte, 1);
    send_packet(uart, &kcs.stop_addr_low_byte, 1);
    send_packet(uart, kcs.data, kcs.length);
    send_packet(uart, &kcs.checksum, 1);

/*
    // clean up
    for (i = 0; i < num_s_record_strings; i++)
    {
        if (s_record[i].data != NULL)
        {
            free(s_record[i].data);
        }
    }
    if (s_record != NULL)
    {
        free(s_record);
    }
    if (kcs.data != NULL)
    {
        free(kcs.data);
    }

    fclose(fp);
*/
    CloseHandle(uart);

    return 0;
}

