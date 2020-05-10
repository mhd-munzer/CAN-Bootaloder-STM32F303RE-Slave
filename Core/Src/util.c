/* utility calls Ymodem
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "util.h"
#include "main.h"


/************************************************************************
 *  Definitions for UART0
 ************************************************************************/

//#define USE_HANDSHAKE                  /* uncomment to use RTS/CTS    */
#define POLLSIZE                0x405    /* 1029 */
#define HW_LIMIT_STOP   (POLLSIZE-64)    /* RTS OFF when 64 chars in buf */
#define HW_LIMIT_START  (POLLSIZE-32)    /* RTS  ON when 32 chars in buf */
#define SERIAL_MCR_DTR           0x01    /* Data Terminal Ready         */
#define SERIAL_MCR_RTS           0x02    /* Request To Send             */
#define SERIAL_LSR_DR            0x01    /* Data ready                  */
#define SERIAL_LSR_THRE          0x20    /* Transmit Holding empty      */
#define SERIAL_MSR_CTS           0x10    /* Clear to send               */


/************************************************************************
 *  Static variables
 ************************************************************************/
static UINT16  recv_buffer[POLLSIZE];
static UINT16 *putptr = &recv_buffer[0];
static UINT16 *getptr = &recv_buffer[0];

/*
 CAN Defenitions
 */
extern CAN_HandleTypeDef hcan;
extern CAN_TxHeaderTypeDef pTxHeader;
extern CAN_RxHeaderTypeDef pRxHeader;
extern uint32_t TxMailBox;
extern uint8_t Txbuffer[8];
extern uint8_t Rxbuffer[8];


void sync()
{
//	asm(".set mips3");
//	asm("sync");
//	asm(".set mips1");
}

/* Be mindful that with a 32 bit register and a 27 MHz clock,
   the maximum timeout value is about 159 seconds */
void _msleep(unsigned long ms)
{
	//replace with hal_delay
	HAL_Delay(ms);
}

void _sleep(unsigned long seconds)
{
	_msleep(seconds*1000);
}

static int serial_poll()
{
	//replace with can pull
}

void serial_write(UINT8  *p_param)
{
	//replace with can write.

	HAL_CAN_AddTxMessage(&hcan, &pTxHeader, p_param, &TxMailBox);

}

int serial_read()
{
//replace with can read
	if (HAL_CAN_GetRxFifoFillLevel(&hcan, CAN_RX_FIFO0) > 0)
		HAL_CAN_GetRxMessage(&hcan, CAN_RX_FIFO0, &pRxHeader, Rxbuffer);
	return (Rxbuffer);
}

/* Get a character with a timeout in seconds. Negative timeout means infinite */
int _getchar(int timeout)
{
	int c;
	unsigned long start_timer = 0, current_timer, end_timer = 0;
	unsigned long long check_overflow;

//	if (timeout >=0) {
//		REG32(RTGALAXY_TIMER_TC2CR) = 0x80000000;
//		start_timer = REG32(RTGALAXY_TIMER_TC2CVR);
//		check_overflow = timeout*CPU_FREQUENCY;
//		if (check_overflow > 0x100000000LL) {
//			end_timer = start_timer-1;
//		} else {
//			end_timer = start_timer + timeout*CPU_FREQUENCY;
//		}
//	}

	do {
		c = serial_read();
		if( c >= 0) {
			return c;
		}
//		if (timeout >= 0) {
//			current_timer = REG32(RTGALAXY_TIMER_TC2CVR);
//			if ( (current_timer > end_timer) &&
//				 ((end_timer >= start_timer) || (current_timer < start_timer)) ) {
//				break;
//			}
//		}
	} while(1);

	return -1;
}

void _putchar(int c)
{
	unsigned char b = c & 0xFF;
	serial_write(&b);
}

void _putc(void* p, char c)
{
	if (c == '\n') {
		c = 0xd;
		serial_write(&c);
		c = 0xa;
	}
	serial_write(&c);
}

void _memset(void *dst, UINT8 value, UINT32 size)
{
	UINT32 i;
	for (i=0; i<size; i++)
		REG8(((UINT32)dst) + i) = value;
}

void _memcpy(void *dst, void *src, UINT32 size)
{
	UINT32 i;
	for (i=0; i<size; i++)
		REG8(((UINT32)dst) + i) = REG8(((UINT32)src) + i);
}

int _strncmp(const char *s1, const char *s2, size_t n) {
	register const char* a=s1;
	register const char* b=s2;
	register const char* fini=a+n;
	while (a<fini) {
		register int res=*a-*b;
		if (res) return res;
		if (!*a) return 0;
		++a; ++b;
	}
	return 0;
}

size_t _strlen(const char* s)
{
	register size_t len = 0;

	if (!s) return 0;
	while (*s++) len++;
	return len;
}

char *_strncpy(char *dest, const char *src, size_t n) {
	_memcpy((void*)dest, (void*)src, n);
	if (n)
		dest[n-1]=0;
	return dest;
}

int isspace(int ch)
{
	return (unsigned int)(ch - 9) < 5u  ||  ch == ' ';
}

unsigned long _strtoul(const char *nptr, char **endptr, int base)
{
	int neg = 0;
	unsigned long v = 0;

	while (isspace(*nptr)) ++nptr;
	if (*nptr == '-') { neg=1; nptr++; }
	if (*nptr == '+') ++nptr;
	if (base==16 && nptr[0]=='0') goto skip0x;
	if (!base) {
		if (*nptr=='0') {
			base=8;
skip0x:
			if (nptr[1]=='x'||nptr[1]=='X') {
				nptr+=2;
				base=16;
			}
		} else
			base=10;
	}
	while (*nptr) {
		register unsigned char c=*nptr;
		c=(c>='a'?c-'a'+10:c>='A'?c-'A'+10:c<='9'?c-'0':0xff);
		if (c>=base) break;
		{
			register unsigned long int w=v*base;
			if (w<v) {
				return ULONG_MAX;
			}
			v=w+c;
		}
		++nptr;
	}
	if (endptr) *endptr=(char *)nptr;
	return (neg?-v:v);
}
