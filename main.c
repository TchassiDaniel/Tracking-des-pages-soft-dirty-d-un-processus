#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/stat.h>

//Cette variable va contenir le nombre d'adresses dans le tableau addresses
int nbAddresses = 0;


//Fonction qui va lire le fichier /proc/$pid/pagemap pour connaitre les adresses
//des zones qui ont été mappée en memoire
int readPagemap(int filePagemap, uint64_t startAddress, uint64_t endAddress){
    int dirtyPage = 0; //Le nombres de page ayant un soft-dirty à 1
    
    for(uint64_t i = startAddress; i < endAddress; i += 0x1000) {
        uint64_t data;
        uint64_t index = (i / 0x1000) * sizeof(data);
        if(pread(filePagemap, &data, sizeof(data), index) != sizeof(data)) {
            perror("pread");
            break;
        }
        if(((data >> 55) & 1) == 1) {
            printf("0x%lx\n", i);
            dirtyPage++;
        }
    }
    return dirtyPage;
}
//Cette fonction va lire le fichier /proc/$pid/maps pour connaitre les adresses
//des zones qui ont été mappé en memoire
int readMaps(char *fileNameMaps, char *fileNamePagemap){
    char buffer[BUFSIZ] = {};
    int i = 0; //Le nombres de page ayant un soft-dirty à 1

        //On ouvre les fichier
        FILE* fileMaps = fopen(fileNameMaps, "r");
        int filePagemap = open(fileNamePagemap, O_RDONLY);
        if(fileMaps == NULL || filePagemap == -1){
            printf("Impossible d'ouvrir le fichier %s\n ou le fichier %s\n", fileNameMaps, fileNamePagemap);

            exit(EXIT_FAILURE);
        }

        //On lit les lignes du fichier maps
        while(fgets(buffer, BUFSIZ, fileMaps) != NULL){
            uint64_t startAddress;
            uint64_t endAddress;

            //Buffer de lecture dans le fichier
            char buffer1[BUFSIZ] = {};
            char buffer2[BUFSIZ] = {};
            //On recupère le nom de la zone mappée en memoire
            sscanf(buffer, "%[^-]-%[^ ]", buffer1, buffer2);
   
            ///On convertit en entier
            startAddress = strtoull(buffer1, NULL, 16);
            endAddress = strtoull(buffer2, NULL, 16);
            //printf("%s", buffer1);
            //printf("Start Adress: %lx End Adresse: %lx\n", startAddress, endAddress);
            //On envoie les adresse a readPagemap
            i+= readPagemap(filePagemap, startAddress, endAddress);
        }
    fclose(fileMaps);
    close(filePagemap);
    return i;
}

int main(){
    char fileNameMaps[BUFSIZ] = {};
    char fileNamePagemap[BUFSIZ] = {};
    char fileNameClear[BUFSIZ] = {};
    char buffer[BUFSIZ] = {};
    
    int fileClear = 0;
    
    int pid = 0;
    int i = 1;
    int nbAddressesPre = 0;
    printf("Veuillez entrer le pid :");

    scanf("%d", &pid);

    if(pid > 0){
        //On recupère le nom du fichier de maps
        sprintf(fileNameMaps, "/proc/%d/maps", pid);
        //On récupère le nom du fichier de pagemap
        sprintf(fileNamePagemap, "/proc/%d/pagemap", pid);
        
        do{
            if (i == 1){
            nbAddresses = readMaps(fileNameMaps, fileNamePagemap);
            printf("Il ya eu %d page accédé initialement\n", nbAddresses);
            i = 0;
            }
            //On met les bit de soft-dirty à 1
            sprintf(fileNameClear, "/proc/%d/clear_refs", pid);
            fileClear = open(fileNameClear, O_WRONLY);
            if(fileClear != -1){
                write(fileClear, "4", sizeof("4"));
                close(fileClear);
            }else{
                printf("Impossible d'ouvrir le fichier %s\n", fileNameClear);
                exit(EXIT_FAILURE);
            }
            nbAddressesPre = nbAddresses;
            nbAddresses = readMaps(fileNameMaps, fileNamePagemap);
            printf("Il ya eu %d page accédé\n", nbAddresses);
            printf("Le rapport est %d", nbAddresses/(nbAddressesPre+1));
            if(0.9 < nbAddresses/(nbAddressesPre+1) && nbAddresses/(nbAddressesPre+1) < 1.1){
                
                printf("Le nombre d'adresse est presque constant, A plus\n");
                
            }
        //
        }while(1);
    }
    else{
        printf("Désolé mais le pid est érrone");
    }

    return 0;
    
}