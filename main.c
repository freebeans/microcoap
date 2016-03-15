/*
 * Mudei esse exemplo pra que o shell interativo rode. No exemplo
 * original, o servidor microcoap rodava imediatamente, sem que o node
 * pudesse encontrar o border router ou mesmo inicializar o RPL. Agora
 * tu pode opcionalmente iniciar o RPL pelo terminal com:
 * 	rpl init <iface>
 * 
 * Também usa ifconfig e espera até aparecer um IPv6 global com o prefixo
 * da rede que teu border router usa. Aí é só pegar esse IPv6 e jogar no
 * Copper. Lembra de iniciar o microcoap no shell com:
 * 	coap
 * 
 * O shell vai rodar o coap pra sempre até ser reiniciado, então é bom
 * configurar tudo antes de rodá-lo.
 * */

#include <stdio.h>
#include "periph/gpio.h"
#include "shell.h"
#include "msg.h"

#define MAIN_QUEUE_SIZE     (8)
static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];
int _microcoap_server_loop(int argc, char **argv);
extern void microcoap_server_loop(void);



/* Essa função é o handler que o shell usa para o comando ifconfig.
 * Todas as funções 'registradas' no shell devem ter esse formato
 * de argumento (argc e argv) pra receberem os argumentos digitados,
 * mesmo que não sejam usados.
 * 
 * Pode ser encontrada em RIOT/sys/shell/commands/sc_netif.c     */
extern int _netif_config(int argc, char **argv);



/* Gambiarra para que o shell consiga chamar microcoap_server_loop().
 * Os argumentos são ignorados. Se tentar chamar a função diretamente
 * (neste caso colocando microcoap_server_loop no callback abaixo), o
 * compilador vai reclamar. */
int _microcoap_server_loop(int argc, char **argv){
	microcoap_server_loop();
	return 0;
}


/* Comando no shell com texto de ajuda e função de callback.
 * Essa estrutura é usada para registrar novos comandos no shell
 * interativo. No caso, eu adicionei a entrada da minha gambiarra.
 * Essa estrutura parece poder conter uma quantidade arbitrária de
 * comandos, mas precisa ser terminada com uma entrada nula.
 * Ela é passada como parâmetro para shell_run(). */
static const shell_command_t shell_commands[] = {
    { "coap", "inicia servidor microcoap", _microcoap_server_loop },
    { NULL, NULL, NULL }
};


int main(void)
{
    puts("RIOT microcoap example application");

    /* microcoap_server uses conn which uses gnrc which needs a msg queue */
    msg_init_queue(_main_msg_queue, MAIN_QUEUE_SIZE);

    /* print network addresses */
    /* Printa os endereços de rede usando aquele handler do ifconfig no shell sem argumentos*/
    puts("Configured network interfaces:");
    _netif_config(0, NULL);


	/* Cria buffer de entrada do shell e invoca o shell registrando o comando coap */
	char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    /* should be never reached */
    return 0;
}
