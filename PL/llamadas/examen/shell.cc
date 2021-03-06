#include <iostream>
#include <string>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

using namespace std;

#define MAX_ARGUMENTOS 20
int loggingOut = 0;

unsigned ObtieneArgumentos(string comando, char *argv[]) {
  // NECESITA: Un sring con la linea de entrada (o comando del usuario)
  // MODIFICA: argv para que tenga el comando en formato de ARGV para el exec()
  // PRODUCE: El número de argumentos de la orden

  string separadores=" \t"; // Caracteres separadores de argumentos
  string apos_comillas="'\""; 
  string palabra; // Contendrá la palabra extraida del string comando
  int comienzo, fin; // Indican que sección del string comando se va tratando
  int comienzo_apos_comillas, fin_apos_comillas; // Indican la posición de unas comillas o apostrofe
  int n_argumento=0; // Indica qué posición de argv vamos tratando

  // Vacía y libera el contenido de argv hasta encontrar NULL
  // Esto se hace para liberar la memoria previamente reservada para el 
  // comando anterior.
  for (int i=0; argv[i] != NULL; i++) {
    free(argv[i]);
    argv[i]=NULL;
  }

  // Bucle que recorre el string comando y obtiene
  // cada una de sus palabras

  // Se salta los separadores iniciales (los que pueda haber
  // al comienzo del string)
  // comienzo queda apuntando al primer caracter de la primera palabra
  comienzo=comando.find_first_not_of(separadores);

  // Mientras no haya llegado al final del string...
  while (comienzo >= 0 && comienzo < comando.size()) {
    // Ya tenemos comienzo aputando al primer caracter
    // de la palabra actual

    // Localiza el comienzo del siguiente separador
    // que será el final de la palabra que estamos tratando
    fin=comando.find_first_of(separadores, comienzo);
    // Comprueba si llego al final. Esto se hace porque la función
    // find_first_of() retorna -1 si no encuentra el siguiente separador
    if (fin == -1)
      // No hemos encontrado el siguiente separador porque era la última
      // palabra. Ponemos fin para que apunte al final del comando
      fin=comando.size();

    // ENTRE comienzo Y fin (EXCLUIDO) ESTÁ LA PALABRA (ARGUMENTO)
    palabra = comando.substr(comienzo, fin-comienzo);

    // Comprobamos si hay apostrofes o comillas
    comienzo_apos_comillas=palabra.find_first_of(apos_comillas);
    if (comienzo_apos_comillas != -1) {
       // Se abrieron apostrofes o comillas, la palabra se extiende hasta su cierre
       // Pero hay que eliminar de la misma el apostrofe o comillas de apertura y cierre
       // Inicializamos palabra con el texto que haya en la palabra actual hasta la apertura
       palabra = comando.substr(0, comienzo_apos_comillas);
       // Localizamos la posisicón de cierre
       fin_apos_comillas = comando.find_first_of(apos_comillas, comienzo+comienzo_apos_comillas+1);
       while (fin_apos_comillas != -1 && comando[comienzo+comienzo_apos_comillas] != comando[fin_apos_comillas]) {
          fin_apos_comillas = comando.find_first_of(apos_comillas, fin_apos_comillas+1);
       }
       // Comprueba si encontró o no el cierre. Si no lo encontró abarcamos todo el texto que queda
       if (fin_apos_comillas == -1) {
          // No hemos encontrado el cierre. Abacamos hasta el final del comando
          fin_apos_comillas=comando.size();
       }
       // Le añadimos todo el texto hasta el cierre
       palabra = palabra + comando.substr(comienzo+comienzo_apos_comillas+1, fin_apos_comillas-(comienzo+comienzo_apos_comillas+1));
       // Le añadimos todo lo que vaya detrás del cierre y esté en la misma palabra
       // Siempre y cuando no hayamos llegado ya al final del comando (por no encontrar el cierre)
       if (fin_apos_comillas != comando.size()) {
          fin=comando.find_first_of(separadores, fin_apos_comillas+1);
          if (fin == -1)
             // No hemos encontrado el siguiente separador porque era la última
             // palabra. Ponemos fin para que apunte al final del comando
             fin=comando.size();
          palabra = palabra + comando.substr(fin_apos_comillas+1, fin-(fin_apos_comillas+1));
       }
       else
          fin=comando.size();
    }

    // Reservamos en la posición actual de argv exactamente el espacio
    // necesario para guardar la palabra actual
    argv[n_argumento] = (char *)malloc(palabra.size()+1);
    // Copiamos la palabra actual a la posición actual de argv
    strcpy(argv[n_argumento], palabra.c_str());
    // Un argumento más
    n_argumento++;
    // Se salta los separadores posteriores a la palabra actual
    // para que comienzo apunte a la siguiente palabra
    comienzo=comando.find_first_not_of(separadores, fin+1);
  }
  // Mete en argv el NULL final que indica al execvp que
  // ya no hay más argumentos
  argv[n_argumento] = NULL;
  // Retorna el número de argumentos
  return n_argumento;
}

void ManejadorLogOut(int s) {
	cout << "Se ha terminado el temporizador de salida." << endl;
	exit(EXIT_SUCCESS);
}

bool ComandoInterno(int mi_argc, char *mi_argv[]) {
   // Los char * son punteros en C/C++ y no se pueden comparar
   // directamente con ==. Es decir, ESTO NO FUNCIONA:
   // if (argv[0] == "exit")
   // Lo que se comprueba en el if de arriba es si la dirección
   // donde está almacenada argv[0] es la misma donde se almacenó
   // la cadena "exit" (lo que siempre retornará FALSO).
   // Lo que realmente queremos es comparar carácter a carácter y ver
   // si son iguales. Esto es precisamente lo que hace la función
   // de librería strcmp().


   if(loggingOut == 1) { // Si hay un temporizador de salir de sesión activo:
	cout << "¡Aviso! ¡Hay un temporizador para salir de sesión activo!" << endl; // Se avisa al usuario.
	sleep(3); // Se retrasa la ejecución del comando para que de tiempo a leer.
   }

   if (strcmp(mi_argv[0], "exit") == 0 || strcmp(mi_argv[0], "logout") == 0)
      exit(EXIT_SUCCESS);

   if (strcmp(mi_argv[0], "logout_temp") == 0) {
	if(loggingOut == 1) { // Si ya existe un temporizador:
		cout << "Ya hay un temporizador para salir de sesión en marcha. Se ignorará la orden actual." << endl; // Se avisa al usuario.
		return true; //  Se devuelve true aunque haya ocurrido un error porque sigue siendo un comando interno, pero se sale.
	}

	// Se comprueba el resto de argumentos.
	if(mi_argc != 2) { // Si el número de argumentos es incorrecto, mostrar uso.
		cout << "Uso: logout_temp <segundos>" << endl;
		return true; // Al igual que en el if anterior, se sale retornando true porque sigue siendo un comando interno.
	}

	loggingOut = 1; // Se guarda el hecho de que hay un temporizador para salir activo.

	// Se crea el manejador para la señal de alarma.
	struct sigaction act;
	act.sa_handler = ManejadorLogOut;
	sigaction(SIGALRM, &act, NULL);

	alarm(atoi(mi_argv[1])); // Se crea la alarma (método no bloqueante)
	return true;
   }

   if (strcmp(mi_argv[0], "cd")==0) {
      char *newdir;

      // Si no se indica ruta de destino se cambia
      // al directorio personal del usuario, indicado
      // por la variable de entorno HOME
      if (mi_argc == 1) {
         newdir = getenv("HOME");
      }
      else
         newdir = mi_argv[1];

      // Utiliza la llamada al sistema chdir para
      // cambiar el directorio por defecto
      if (chdir(newdir) == -1) 
         // Hubo un error
         perror("Error cd");

      return true;
   } // Del comando interno cd

   // Llega aquí si no entró por ninguno de los if anteriores 
   // (por ningún comando interno)
   return false;
}

int main() {
  string comando;
  string el_prompt="$";
  char *comando_argv[MAX_ARGUMENTOS];
  int n_args, pid_hijo;

  // Inicializa comando_argv
  for (int i=0; i<MAX_ARGUMENTOS; i++)
    comando_argv[i]=NULL;

  // Bucle principal del shell. Siempre lee de la entrada estándar el siguiente comando
  // hasta que encuentre fin de fichero O SE EJECUTEN LOS COMANDOS INTERNOS exit ó logout
  while (!cin.eof()) {
    // Imprime el prompt
    cout << el_prompt << " ";

    // Lee una linea desde la entrada estándar
    getline(cin, comando);

    // Separa el comando en ejecutable más argumentos.
    n_args=ObtieneArgumentos(comando, comando_argv);
    
    if (n_args != 0) {
      // El siguiente if trata de ejecutar el comando introducido
      // como un comando interno. Si el comando interno existe
      // se retornará CIERTO y FALSO si no existe.
      // En este último caso (si retorna FALSO) intenta entonces
      // ejecutarlo como comando externo (mediante exec)   
      if (!ComandoInterno(n_args, comando_argv)) { 
       // ES UN COMANDO EXTERNO
       // Ejecuta el comando indicado por el usuario
       pid_hijo = fork();
       if (pid_hijo == 0) {
          // Código del hijo
          execvp(comando_argv[0], comando_argv);
          cerr << comando_argv[0] << ": Comando no encontrado." << endl;
          exit(EXIT_FAILURE);
       }
       // Codigo del padre
       wait(NULL);
    } // Fin de if (!ComandoInterno
   } // De if n_args != 0
  } // Del while (!eof)
}
