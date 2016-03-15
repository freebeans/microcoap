#include "net/af.h"
#include "net/conn/udp.h"

#ifdef MICROCOAP_DEBUG
#define ENABLE_DEBUG (1)
#else
#define ENABLE_DEBUG (0)
#endif
#include "debug.h"

#include "coap.h"

static uint8_t _udp_buf[512];   /* udp read buffer (max udp payload size) */
uint8_t scratch_raw[1024];      /* microcoap scratch buffer */

coap_rw_buffer_t scratch_buf = { scratch_raw, sizeof(scratch_raw) };

#define COAP_SERVER_PORT    (5683)

/*
 * Starts a blocking and never-returning loop dispatching CoAP requests.
 *
 * When using gnrc, make sure the calling thread has an initialized msg queue.
 */
void microcoap_server_loop(void)
{

    uint8_t laddr[16] = { 0 };
    uint8_t raddr[16] = { 0 };
    size_t raddr_len;
    uint16_t rport;

    conn_udp_t conn;

	/* Cria conexão UDP.
	 * &conn é o objeto de conexão.
	 * laddr é o endereço local. Como é zero, o endereço na conexão será
	 * 		não especificado (conn->addr = 0);
	/* Retorna 0 em caso de sucesso. */

    int rc = conn_udp_create(&conn, laddr, sizeof(laddr), AF_INET6, COAP_SERVER_PORT);

    while (1) {
        DEBUG("Waiting for incoming UDP packet...\n");
        
        /* Espera uma conexão UDP e joga o pacote recebido em _udp_buf.
         * Retorna o número de bytes lidos ou zero (se tudo estiver OK).
         * Qualquer número negativo é um erro. Aparentemente é compatível
         * com os valores de errno setados pela função recvfrom() POSIX. */
        rc = conn_udp_recvfrom(&conn, (char *)_udp_buf, sizeof(_udp_buf), raddr, &raddr_len, &rport);
        if (rc < 0) {
            DEBUG("Error in conn_udp_recvfrom(). rc=%u\n", rc);
            continue;
        }

		/* Quantidade de bytes lidos */
        size_t n = rc;

        coap_packet_t pkt;
        DEBUG("Received packet: ");
        
        /* Printa o pacote em formato hexadecimal */
        /* Função definida em RIOT/pkg/microcoap/microcoap/coap.c:25 */
        coap_dump(_udp_buf, n, true);
        DEBUG("\n");

        /* Completa o pacote coap PKT com as informações lidas do buffer.
         * Retorna 0 em caso de sucesso. Caso contrário, retorna um erro
         * definido em RIOT/pkg/microcoap/microcoap/coap.h:112 */
        /* Função definida em RIOT/pkg/microcoap/microcoap/coap.c:214 */
        if (0 != (rc = coap_parse(&pkt, _udp_buf, n))) {
            DEBUG("Bad packet rc=%d\n", rc);
        }
        else {
            coap_packet_t rsppkt;
            DEBUG("content:\n");
            /* Função definida em RIOT/pkg/microcoap/microcoap/coap.c:204 */
            coap_dumpPacket(&pkt);

			
			/* Escaneia os endpoints criados em coap.c (local)
			 * e chama o handler cujo caminho bater. */
            /* Função definida em RIOT/pkg/microcoap/microcoap/coap.c:395 */
            coap_handle_req(&scratch_buf, &pkt, &rsppkt);

            /* build reply */
            size_t rsplen = sizeof(_udp_buf);
            
            /* Constrói o pacote coap. Pega o rsppkt gerado pelo handler
             * do endereço anteriormente e constrói no buffer. O formato
             * do pacote é definido em:
             * http://tools.ietf.org/html/draft-ietf-core-coap-18#section-3
             * 
             * Função definida em RIOT/pkg/microcoap/microcoap/coap.c:265 */
            if ((rc = coap_build(_udp_buf, &rsplen, &rsppkt)) != 0) {
                DEBUG("coap_build failed rc=%d\n", rc);
            }
            else {
                DEBUG("Sending packet: ");
                coap_dump(_udp_buf, rsplen, true);
                DEBUG("\n");
                DEBUG("content:\n");
                coap_dumpPacket(&rsppkt);

                /* send reply via UDP */
                rc = conn_udp_sendto(_udp_buf, rsplen, NULL, 0, raddr, raddr_len, AF_INET6, COAP_SERVER_PORT, rport);
                if (rc < 0) {
                    DEBUG("Error sending CoAP reply via udp; %u\n", rc);
                }
            }
        }
    }
}
