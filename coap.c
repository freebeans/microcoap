#include <coap.h>
#include <string.h>
#include "periph/gpio.h"

#define MAX_RESPONSE_LEN 500
static uint8_t response[MAX_RESPONSE_LEN] = { 0 };

/* Uma explicação MUITO boa de ler sobre o que acontece aqui:
 * http://watr.li/microcoap-and-ff-copper.html */

/* Handlers para os endpoints (endereços, recursos...) 
 * São chamados por coap_handle_req() localizada em RIOT/pkg/microcoap/microcoap/coap.c */

static int handle_get_well_known_core(coap_rw_buffer_t *scratch,
                                      const coap_packet_t *inpkt,
                                      coap_packet_t *outpkt,
                                      uint8_t id_hi, uint8_t id_lo);

static int handle_get_riot_board(coap_rw_buffer_t *scratch,
                                 const coap_packet_t *inpkt,
                                 coap_packet_t *outpkt,
                                 uint8_t id_hi, uint8_t id_lo);




/* Definições de endpoints (os endereços dos recursos)
 * Por default o RIOT só suporta 2 'níveis' de profundidade, mas tu pode
 * definir mais mudando MAX_SEGMENTS em RIOT/pkg/microcoap/microcoap/coap.h */

static const coap_endpoint_path_t path_well_known_core =
        { 2, { ".well-known", "core" } };

static const coap_endpoint_path_t path_riot_board =
        { 2, { "riot", "board" } };
        
static const coap_endpoint_path_t path_led =
		{1, { "led" } };




/*
 * No arquivo RIOT/pkg/microcoap/microcoap/coap.h
 * 
 * A função coap_handle_req() definida em RIOT/pkg/microcoap/microcoap/coap.c
 * espera uma array de endpoints chamada "endpoints" externa.
 * 
typedef struct
{
    coap_method_t               method;				// GET, PUT, POST, etc...
    coap_endpoint_func          handler;			// Função de callback para a requisição que chegar...
    const coap_endpoint_path_t *path;				// ...neste recurso.
    const char                 *core_attr;			// Atributo CT definido no RFC7252, seção 7.2.1
    * 													Tem alguma coisa aqui também: http://tools.ietf.org/html/rfc6690
    * 													Ele serve como um Content-type HTTP codificado númericamente pra reduzir o tamanho.
    * 														Tem uma lista de definições aqui:
    * 														http://tools.ietf.org/html/draft-ietf-core-coap-18#section-12.3
    * 
    * 													Tem uma enumeração definida em RIOT/pkg/microcoap/microcoap/coap.h:103 com alguns
    * 														tipos (os dois usados pelos handlers do exemplo (zero e quarenta)).
    * 
    * 													coap_make_response() simplesmente joga esse valor na resposta. Quem interpreta ele
    * 														e decide o que fazer com o payload deve ser quem inicia a requisição (assim como HTTP).
} coap_endpoint_t; */


const coap_endpoint_t endpoints[] =
{
    { COAP_METHOD_GET,	handle_get_well_known_core,
        &path_well_known_core, "ct=40" },
        
    { COAP_METHOD_GET,	handle_get_riot_board,
        &path_riot_board,	   "ct=0"  },
        
    { COAP_METHOD_GET, 	handle_post_led,
		&path_led,	 		   "ct=0"},		// content-type: texto (COAP_CONTENTTYPE_TEXT_PLAIN)
    /* marks the end of the endpoints array: */
    { (coap_method_t)0, NULL, NULL, NULL }
};






static int handle_post_led(coap_rw_buffer_t *scratch,
        const coap_packet_t *inpkt, coap_packet_t *outpkt,
        uint8_t id_hi, uint8_t id_lo)
{
	static uint8_t estado_led = 0;
	int mensagem_len[4];
	int final_msg_len;
	
	enum{TROCOU=0, NTROCOU, ON, OFF};
	
	const char * mensagens[] = {"Estado do LED trocou para ",
								 "Estado do LED já era ",
								 "ligado.",
								 "desligado."};
								 
	mensagem_len[TROCOU] = strlen(mensagens[TROCOU]);
	mensagem_len[NTROCOU] = strlen(mensagens[NROCOU]);
	mensagem_len[ON] = strlen(mensagens[ON]);
	mensagem_len[OFF] = strlen(mensagens[OFF]);
	
	enum{CM_LED_TOGGLE, CM_LED_ON, CM_LED_OFF};
	
	printf("handle_post_led - payload: %s\n", inpkt->payload->p);
	
	/* Não sei se o payload termina com \0 ou \n, então preciso checar */
	if(inpkt->payload->len > 2){
	
	/* Se trocou o estado, envia "Estado do LED trocou para <estado>."
	 * Se não trocou estado,     "Estado do LED já era <estado>."
	 * 
	 * Não ficou muito elegante. */
	
	
		if( strncmp( inpkt->payload->p, "toggle", 2 )==0 )
		{
			if(estado_led)
				estado_led = 0;
			else
				estado_led = 1;
				
			memcpy(response, mensagens[TROCOU], mensagem_len[TROCOU]);
			memcpy((response+TROCOU), mensagens[ estado_led?ON:OFF ], mensagem_len[ estado_led?ON:OFF ]);
			final_msg_len = TROCOU+(estado_led?ON:OFF);
		}
		else if( strncmp( inpkt->payload->p, "on", 2 ) == 0)
		{
			if(!estado_led){
				memcpy(response, mensagens[TROCOU], mensagem_len[TROCOU]);
				memcpy((response+TROCOU), mensagens[ON], mensagem_len[ON]);
				final_msg_len = TROCOU+ON;
			}else{
				memcpy(response, mensagens[NTROCOU], mensagem_len[NTROCOU]);
				memcpy((response+NTROCOU), mensagens[ON], mensagem_len[ON]);
				final_msg_len = NTROCOU+ON;
			}
			estado_led = 1;
		}
		else if( strncmp( inpkt->payload->p, "off", 2 ) == 0)
		{
			if(estado_led){
				memcpy(response, mensagens[TROCOU], mensagem_len[TROCOU]);
				memcpy((response+TROCOU), mensagens[OFF], mensagem_len[OFF]);
				final_msg_len = TROCOU+OFF;
			}else{
				memcpy(response, mensagens[NTROCOU], mensagem_len[NTROCOU]);
				memcpy((response+NTROCOU), mensagens[OFF], mensagem_len[OFF]);
				final_msg_len = NTROCOU+OFF;
			}
			estado_led = 0;
		}
		
		if(estado_led)
			LED_ON;
		else
			LED_OFF;
			
	}
	
	return coap_make_response(scratch, outpkt, (const uint8_t *)response, final_msg_len,
                              id_hi, id_lo, &inpkt->tok, COAP_RSPCODE_CONTENT,
                              COAP_CONTENTTYPE_TEXT_PLAIN);
}







static int handle_get_well_known_core(coap_rw_buffer_t *scratch,
        const coap_packet_t *inpkt, coap_packet_t *outpkt,
        uint8_t id_hi, uint8_t id_lo)
{
    char *rsp = (char *)response;
    int len = sizeof(response);
    const coap_endpoint_t *ep = endpoints;
    int i;

    len--; // Null-terminated string

    while (NULL != ep->handler) {
        if (NULL == ep->core_attr) {
            ep++;
            continue;
        }

        if (0 < strlen(rsp)) {
            strncat(rsp, ",", len);
            len--;
        }

        strncat(rsp, "<", len);
        len--;

        for (i = 0; i < ep->path->count; i++) {
            strncat(rsp, "/", len);
            len--;

            strncat(rsp, ep->path->elems[i], len);
            len -= strlen(ep->path->elems[i]);
        }

        strncat(rsp, ">;", len);
        len -= 2;

        strncat(rsp, ep->core_attr, len);
        len -= strlen(ep->core_attr);

        ep++;
    }

    return coap_make_response(scratch, outpkt, (const uint8_t *)rsp,
                              strlen(rsp), id_hi, id_lo, &inpkt->tok,
                              COAP_RSPCODE_CONTENT,
                              COAP_CONTENTTYPE_APPLICATION_LINKFORMAT);
}

static int handle_get_riot_board(coap_rw_buffer_t *scratch,
        const coap_packet_t *inpkt, coap_packet_t *outpkt,
        uint8_t id_hi, uint8_t id_lo)
{
    const char *riot_name = RIOT_BOARD;
    int len = strlen(RIOT_BOARD);

    memcpy(response, riot_name, len);

    return coap_make_response(scratch, outpkt, (const uint8_t *)response, len,
                              id_hi, id_lo, &inpkt->tok, COAP_RSPCODE_CONTENT,
                              COAP_CONTENTTYPE_TEXT_PLAIN);
}
