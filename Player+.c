#include <stdio.h>		// string.h pour les caractères, time.h pour les nombres aléatoires
#include <stdlib.h>		// math.h !! ne pas oublier de linker le gcc avec '-lm'
#include <string.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_image.h>

// Lien pour Google drive : https://drive.google.com/open?id=0B_VDBb3XX45VZVhNRHZCdWRrU2s

/*
!!  En fait, il faudrait pas copier tout les pointeurs à chaque fois. Il faudrait créer une structure "voix", et mettre
juste son pointeur une fois. Simple et efficace. Ca ne poserait même pas trop de difficultés pour jouer.
		==> Non en fait on va rentabiliser P->modele =)

*/
// Ordre : C2, D2, E2, F2, G2, A2, B2, C3, D3, E3, F3, G3, A3, B3, C4, D4...

#define NBRE_VOIX 20
#define CAD_PARFAITE 0
#define CAD_PLAGALE 1//4->1
#define CAD_COMPLETE 2//6->4->5->1
#define CAD_DEUXIEME 3//1->2
#define CAD_ACCRAND 4
#define N_I 10

typedef struct note {
	Mix_Chunk* num[7];
	} note;

typedef struct mesure {
	int r[NBRE_VOIX][32];// !! 20 parties de 32
	int n[NBRE_VOIX][32];// !! 20 parties de 32
	int nbre[NBRE_VOIX];
	int acc;
	int cadNum, cadPos;// /!\ cadPos dcroissant !!
	} mesure;//  Stocke des rythmes pour une mesure

typedef struct bankr {
	int num[5];
	int casse;// : nbre de temps du rythme (à cause des rondes et des blanches)
	} bankr;//  Stocke des rythmes pour un temps

typedef struct bouton {
	SDL_Surface* cadre;
	SDL_Rect coord;
	} bouton;

typedef struct boutp {
	SDL_Surface* titre;
	SDL_Surface *plus;
	SDL_Surface *moins;
	SDL_Surface *valeur;
	SDL_Rect coord, coordp, coordm, coordv;
	} boutp;

typedef struct parametres {
	Mix_Chunk* err;
	note deg[N_I][12];
	note modele[N_I][12];
	int attr[N_I], attr2[N_I]/*contient le n° de l'instrument*/;
	int nbreInstru;// attribution des instruments, en nombre de voix ( le 1er a 3 voix, le 2eme 4 etc)
	int stateVoix[N_I];// pour savoir si on l'a déja Mix_FreeChunk'ée
	int tessb[N_I], tessh[N_I];// Les bornes sont inclues
	char nomInstru[N_I][20];//Max N_I noms de 20 lettres
	int volInstru[N_I];//volume attribué, en % (tranches de 5)
	int notefreq[9];
	int rythfreq[NBRE_VOIX+4][7];//car 6 rythmes disponibles+ -1 de fin de chaine
	int cadfreq[4];//==> define... (pour les numeros de cadence)
	int bankn[500], bankry[NBRE_VOIX][500], bankcad[500];// !! En 4 parties de 500... A la fin de chaque partie, -1.
	bankr R[6];
	int tempoBPM, mspn;
	int tonalite, mode;
	int nbreVoix, paramVoix[NBRE_VOIX];// 0, 1, 2 ou 3 voix
	int cadon;//      \_> 1 : bassforcée (1er degre)
	} parametres;


int loadgamme (parametres *P);
int randmes (mesure* M, mesure *aM, parametres *P, int numVoix, int accForced);
int playclassic (SDL_Surface* fond, SDL_Window* fen, parametres *P);
int inbout (SDL_Event event, bouton bout);
int inbout2 (SDL_Event event, SDL_Rect coord, SDL_Surface* cadre);
int inparam (parametres *P, SDL_Surface *fond, SDL_Window* fen);
int flipboutp (boutp* bout);

int main (int argc, char *argv[])
{
	int i = 0, j = 0, k = 0, l = 0;
	char c = 100;
	int  youpi[40];
//	int gamme = 3;   	appartiens à [0;11] : 12 tonalités avec tonalité 0 = a; puis +1 -> +1 demi-ton <--<--<--<--
//	int mode = 0; 	0 majeur; 1 mineur antique; 2 mineur melodique
	int continuer = 1;
	int defrythfreq[4][6];// Pq pas à la base dans un fichier txt ==> DONE
	int defnotefreq[] = {0, 18, 2, 5, 12, 15, 10, 1, -1};
	parametres P;

	FILE* source = fopen ("Default.txt", "r");

    srand(time(NULL));
	SDL_Init(SDL_INIT_AUDIO|SDL_INIT_VIDEO);
	TTF_Init();
	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 1024) == -1)
		printf("%s",Mix_GetError());
	Mix_AllocateChannels(20);
    SDL_Surface *fond, *icon = IMG_Load("RM.png");
    SDL_Window *fen;
    fen = SDL_CreateWindow("RandMus", 100, 100, 700, 432, SDL_WINDOW_SHOWN);
    SDL_SetWindowIcon(fen, icon);
	fond=SDL_GetWindowSurface(fen);
    SDL_ShowWindow(fen);//InitialisationSDL

    SDL_Event event;
	bouton bjouer, bquitter, bparam, brm, bfarce[7];
	SDL_Color bleu = {10, 10, 200}, orange = {175, 90,10}, noir = {0,0,0};
	TTF_Font *police2;
	police2 = TTF_OpenFont ("Police.TTF",40);

    SDL_FillRect(fond, NULL, SDL_MapRGB(fond->format, 10, 10, 200));
    brm.cadre = TTF_RenderText_Shaded (police2, "Chargement...", orange, bleu);
    brm.coord.x = 60;
    brm.coord.y = 20;
    SDL_BlitSurface (brm.cadre, NULL, fond, &brm.coord);
    SDL_UpdateWindowSurface(fen);// Affichage :chargement


	for (i = 0; i < 40; i++)
		{
		while (c != '=')
			fscanf (source, "%c", &c);
		fscanf (source, "%c", &c);
		fscanf (source, "%d", &youpi[i]);//Récupération des nombres dans youpi
		}

	for (i = 0; i < 4; i++)
		while (c != '*')
			fscanf (source, "%c", &c);
	while (c != '\n')
		fscanf (source, "%c", &c);
	fscanf (source, "%c", &c);// tt ca pour arriver au bon endroit
	for (i = 0; i <= N_I; i++)
		P.attr[i] = 0;

	for (i = 0; i < N_I; i++)
		{
		fscanf (source, "%s", P.nomInstru[i]);		l = 0;
		while (c != '=' && l < 100)
			{fscanf (source, "%c", &c);
			l++;}//sécurisé =)
		if (l > 99)
			{
			P.nbreInstru = i;
			i = N_I + 1;
			P.nomInstru[i][0] = '\0';
			}
		fscanf (source, "%c", &c);//pour se placer devant le nombre
		fscanf (source, "%d", &j);
		P.attr[i] = j;

		l = 0;
		while ( c != '=' && l < 100)
			{fscanf (source, "%c", &c);
			l++;}
		fscanf (source, "%c", &c);
		fscanf (source, "%d", &P.volInstru[i]);//Récupération des volumes
		l = 0;
		while (c != '\n' && l < 100)
			{fscanf (source, "%c", &c);
			l++;}//sécurisé =)
		}
	k = 0;
	for (i = 0; k < N_I; i++)
		for (j = 0; j < P.attr[i] && k < N_I; j++)
			{
			P.attr2[k] = i;
//			printf ("P.attr2[%d] = %d\ti = %d\n", k, P.attr2[k], i);
			k++;
			}
	printf ("P.nomInstru[0] : *%s*\n", P.nomInstru[0]);

	printf ("youpi5 : %d, youpi6 : %d, youpi7 : %d\n",abs (youpi[5])%(500/7), youpi[6], youpi[7]);
	P.tonalite = abs (youpi[1]) % 12;
	P.mode = abs (youpi[2]) % 4;
	P.tempoBPM = abs (youpi[3]) % 300;
	if (P.tempoBPM == 0)
		P.tempoBPM = 10;
	P.mspn = 60000/P.tempoBPM;
	P.nbreVoix = 1;
	P.cadon = 0;
	for (i = 0; i < 6; i++)
		for (j = 0; j < 4; j++)
			defrythfreq[j][i] = abs (youpi[4+i*4+j]) % (500 / 6);


	for (i = 1; i < 8; i++)
		defnotefreq[i] = abs (youpi[27+i]) % 150;//Remise ds nbres dans youpi à leur places...
	defnotefreq[8] = -1;		    // --> Définition de la tonalité, du mode, du tempo et des fréquences

	for (i = 0; i < 5; i++)
		P.cadfreq[i] = abs (youpi[35+i])%150;


	for (i = 0; i < 6; i++)
		{
		P.rythfreq[NBRE_VOIX + 0][i] = defrythfreq[0][i];
		P.rythfreq[NBRE_VOIX + 1][i] = defrythfreq[1][i];
		P.rythfreq[NBRE_VOIX + 2][i] = defrythfreq[2][i];
		P.rythfreq[NBRE_VOIX + 3][i] = defrythfreq[3][i];
		P.rythfreq[0][i] = defrythfreq[0][i];
		P.rythfreq[1][i] = defrythfreq[1][i];
		P.rythfreq[2][i] = defrythfreq[2][i];
		P.rythfreq[3][i] = defrythfreq[3][i];
		}

	for (i = 0; i < 6; i++)
		for (j = 4; j < NBRE_VOIX; j ++)
			P.rythfreq[j][i] = defrythfreq[3][i];
	for (i = 0; i < NBRE_VOIX; i++)
		P.paramVoix[i] = 0;

	for (i = 0; i < 9; i++)
		P.notefreq[i] = defnotefreq[i];//Définition par défaut des valeurs des fréquences

	k = 0;
	const int banque[] = {1, 0,0,0, 2, 0, 0, 0, 4, 0, 0, 0, 8, 8, 0, 0, 6, 16, 0, 0, 16, 16, 16, 16, -1};
	for (i = 0; i < 6; i++)
		{
		P.R[i].num[0] = banque[4*i];
		P.R[i].num[1] = banque[4*i+1];
		P.R[i].num[2] = banque[4*i+2];
		P.R[i].num[3] = banque[4*i+3];
		P.R[i].num[4] = -1;
		if (P.R[i].num[0] < 4)
			P.R[i].casse = 4/P.R[i].num[0];
		else
			P.R[i].casse = 1;
		}

	for (l = 0; l < 4; l++)
		{
		k = 0;
		for (i = 0; i < 6; i++)
			{
			for (j = 0; j < P.rythfreq[l][i] &&  j < 500/7/*7degrés, 500places*/; j++)	{
				P.bankry[l][k] = i;//	Initialisation de bankry
				k++;									}
			}
		P.bankry[l][k] = -1;
		}
	k = 0;
	for (i = 1; i < 8; i++)
		{
		for (j = 0; j < P.notefreq[i] && j < 500/6/*6rythmes, 500 places*/; j++) 	{
			P.bankn[k] = i;
			printf ("%d,",P.bankn[k]);
			k++;				}
		}
	printf("\n");

//			Initialisation des rythmes et de leurs frquence dans P.rythfreq

	k = 0;
	for (i = 0; i < 4; i++)
		{
		for (j = 0; j < P.cadfreq[i]; j++)	{
			P.bankcad[k] = i;
			k++;				}
		}
	for (i = 0; i < N_I; i++)
		{
		P.tessh[i] = -1;
		P.tessb[i] = -1;
		}

	loadgamme (&P);

/*
Remarque : les degre ont leur n°  par rapport au voc musical : degre[1] = tonique ; degré[5] = dominante...

**************************************************************************************************************************
*/


//____________________________________________________________________________________________________________


	bjouer.cadre = TTF_RenderText_Shaded(police2,"Jouer",noir, orange);
	bjouer.coord.x = 700/2 - (bjouer.cadre->w/2);
	bjouer.coord.y = 432/2 - (bjouer.cadre->h/2)-40;


	bparam.cadre = TTF_RenderText_Shaded(police2,"Parametres",noir, orange);
	bparam.coord.x = 700/2 - (bparam.cadre->w/2);
	bparam.coord.y = 432/2 - (bparam.cadre->h/2) + bjouer.cadre->h - 20;

	bquitter.cadre = TTF_RenderText_Shaded(police2,"Quitter",noir, bleu);
	bquitter.coord.x = 700 - bquitter.cadre->w - 10;
	bquitter.coord.y = 10;

	brm.cadre = TTF_RenderText_Shaded(police2,"RandMus",noir, orange);
	brm.coord.x = 35;
	brm.coord.y = 15;

	bfarce[0].cadre = TTF_RenderText_Shaded(police2,"Tu avais une chance sur dix pour",noir, bleu);
	bfarce[1].cadre = TTF_RenderText_Shaded(police2,"que ce message stupide apparaisse.",noir, bleu);
	bfarce[2].cadre = TTF_RenderText_Shaded(police2,"C'est tout. Il s'en ira.",noir, bleu);
	bfarce[3].cadre = TTF_RenderText_Shaded(police2,"Mais...",noir, bleu);
	bfarce[4].cadre = TTF_RenderText_Shaded(police2,"N'oublie surtout pas...",noir, bleu);
	bfarce[5].cadre = TTF_RenderText_Shaded(police2,"STAY DETERMINED",noir, bleu);
	for (i = 0; i < 6; i++)
		{bfarce[i].coord.x = 25;
		bfarce[i].coord.y = 25 + 50*i;}

	if (rand()%(10) == 0)
		{
		SDL_FillRect (fond, NULL, SDL_MapRGB (fond->format, 10, 10, 200));
		for (i = 0; i < 6; i++)
			SDL_BlitSurface(bfarce[i].cadre, NULL, fond, &bfarce[i].coord);
		SDL_UpdateWindowSurface (fen);
		i = SDL_GetTicks(); j = 0;
		while (SDL_GetTicks() - i < 15000 && j != 1)
			{
			SDL_PollEvent (&event);
			if (event.key.keysym.sym == 's')
				j = 1;
			SDL_Delay (10);
			}
		if (j == 1)
			{
			SDL_FillRect (fond, NULL, SDL_MapRGB (fond->format, 10, 10, 200));
			bfarce[2].cadre = TTF_RenderText_Shaded(police2,"'S' comme Sans. Evidemment.",noir, bleu);
			SDL_BlitSurface(bfarce[2].cadre, NULL, fond, &bfarce[2].coord);
			SDL_UpdateWindowSurface (fen);
			SDL_Delay (10000);
			}
		}


//____________________________________________________________________________________________________________


	while (continuer)
		{
        SDL_FillRect (fond, NULL, SDL_MapRGB (fond->format, 10, 10, 200));
        SDL_BlitSurface (bjouer.cadre, NULL, fond, &bjouer.coord);
        SDL_BlitSurface (bquitter.cadre, NULL, fond, &bquitter.coord);
        SDL_BlitSurface (bparam.cadre, NULL, fond, &bparam.coord);

	SDL_UpdateWindowSurface (fen);

		SDL_WaitEvent (&event);
		switch (event.type)
			{
			case SDL_QUIT:
				continuer = 0;
				break;
			case SDL_MOUSEBUTTONUP:
				{
				if (inbout (event, bquitter))
					continuer = 0;
				else if (inbout (event, bjouer))
					{
					playclassic (fond, fen, &P);
					SDL_FillRect (fond, NULL, SDL_MapRGB (fond->format, 10, 10, 200));
					SDL_BlitSurface (bjouer.cadre, NULL, fond, &bjouer.coord);
					SDL_BlitSurface (bquitter.cadre, NULL, fond, &bquitter.coord);
					SDL_BlitSurface (bparam.cadre, NULL, fond, &bparam.coord);

					SDL_UpdateWindowSurface (fen);
					}
				else if (inbout (event, bparam))
					{
					inparam (&P, fond, fen);
					SDL_FillRect (fond, NULL, SDL_MapRGB (fond->format, 10, 10, 200));
					SDL_BlitSurface (bjouer.cadre, NULL, fond, &bjouer.coord);
					SDL_BlitSurface (bquitter.cadre, NULL, fond, &bquitter.coord);
					SDL_BlitSurface (bparam.cadre, NULL, fond, &bparam.coord);
					SDL_UpdateWindowSurface (fen);
					}
				}
			}
		}

    SDL_DestroyWindow(fen);
	Mix_CloseAudio();
	SDL_Quit();

	return 0;
}


/*
/\___/\	/\___/\	/\___/\	/\___/\	/\___/\
\ ô ô /	\ ô ô /	\ ô ô /	\ ô ô /	\ oO  /
/  B  \	/  I  \	/  S  \	/  O  \	/  U  \
\_____/	\_____/	\_____/	\_____/	\_____/
*/

int loadgamme (parametres *P)
{
	char suite[26], nomnote[20];
	P->err = Mix_LoadWAV("Error.wav");
	if (P->err == NULL)
		printf("\n erreur au chargement du bruit d'erreur :%s", Mix_GetError());
	else
		Mix_VolumeChunk(P->err, MIX_MAX_VOLUME/2);
	int i = 0, j = 0, k = 0, l = 0, m = 0, ppte = -1;
	note N[N_I][12];  	//N_I instruments et 12 notes (en chroma)
	for (i = 0; i < N_I; i++)
		for (j = 0; j < 12; j++)
			for (k = 0; k < 7; k++)
				N[i][j].num[k] = NULL;
	i = 0; j = 0; k = 0;
	switch (P->tonalite)
		{
		case 0:
			if (P->mode == 0)		sprintf (suite,"a b c#d e f#g#");// 0 : Majeur
			else if (P->mode == 1)		sprintf (suite,"a b c d e f g ");// 1 : Mineur
			else if (P->mode == 2)		sprintf (suite,"a b c d e f#g#");// 2 : Mélodique
		break;									 // 3 : Chromatique
		case 1:
			if (P->mode == 0)		sprintf (suite,"a#c d d#f g a ");
			else if (P->mode == 1)		sprintf (suite,"a#c c#d#f f#g#");
			else if (P->mode == 2)		sprintf (suite,"a#c c#d#f g a ");
		break;
		case 2:
			if (P->mode == 0)		sprintf (suite,"b c#d#e f#g#a#");
			else if (P->mode == 1)		sprintf (suite,"b c#d e f#g a ");
			else if (P->mode == 2)		sprintf (suite,"b c#d e f#g#a#");
		break;
		case 3:
			if (P->mode == 0)		sprintf (suite,"c d e f g a b ");
			else if (P->mode == 1)		sprintf (suite,"c d d#f g g#a#");
			else if (P->mode == 2)		sprintf (suite,"c d d#f g a b ");
		break;
		case 4:
			if (P->mode == 0)		sprintf (suite,"c#d#f f#g#a#c ");
			else if (P->mode == 1)		sprintf (suite,"c#d#e f#g#a b ");
			else if (P->mode == 2)		sprintf (suite,"c#d#e f#g#a#c ");
		break;
		case 5:
			if (P->mode == 0)		sprintf (suite,"d e f#g a b c#");
			else if (P->mode == 1)		sprintf (suite,"d e f g a a#c ");
			else if (P->mode == 2)		sprintf (suite,"d e f g a b c#");
		break;
		case 6:
			if (P->mode == 0)		sprintf (suite,"d#f g g#a#c d ");
			else if (P->mode == 1)		sprintf (suite,"d#f f#g#a#b c#");
			else if (P->mode == 2)		sprintf (suite,"d#f f#g#a#c d ");
		break;
		case 7:
			if (P->mode == 0)		sprintf (suite,"e f#g#a b c#d#");
			else if (P->mode == 1)		sprintf (suite,"e f#g a b c d ");
			else if (P->mode == 2)		sprintf (suite,"e f#g a b c#d#");
		break;
		case 8:
			if (P->mode == 0)		sprintf (suite,"f g a a#c d e ");
			else if (P->mode == 1)		sprintf (suite,"f g g#a#c c#d#");
			else if (P->mode == 2)		sprintf (suite,"f g g#a#c d e ");
		break;
		case 9:
			if (P->mode == 0)		sprintf (suite,"f#g#a#b c#d#f ");
			else if (P->mode == 1)		sprintf (suite,"f#g#a b c#d e ");
			else if (P->mode == 2)		sprintf (suite,"f#g#a b c#d#f ");
		break;
		case 10:
			if (P->mode == 0)		sprintf (suite,"g a b c d e f#");
			else if (P->mode == 1)		sprintf (suite,"g a a#c d d#f ");
			else if (P->mode == 2)		sprintf (suite,"g a a#c d e f#");
		break;

		case 11:
			if (P->mode == 0)		sprintf (suite,"g#a#c c#d#f g ");
			else if (P->mode == 1)		sprintf (suite,"g#a#b c#d#e f#");
			else if (P->mode == 2)		sprintf (suite,"g#a#b c#d#f g ");
		break;
		}


	while (i < 7)
		{
		if (suite[2*i] == 'c' && suite[2*i+1] == ' ')
			{
			ppte = i;
			i = 7;
			}
		else if (suite[2*i] == 'c' && suite[2*i+1] != ' ')
			ppte = i;
		i++;
		}//ppte : degré du do/do# le plus bas dans suite, e [0,6]

	for (k = 0; k < P->nbreInstru; k++)
		{
		i = ppte - 1;//degré
		j = 0;//N° (le do 0 n'existe pas, mais à cause du if (i = ppte), "on commence plus tôt")

		while (N[k][i+1].num[j] == NULL && j < 20)
			{
			i++;
			if (i > 6)
				i = 0;
			if (i == ppte)
				j++;

			if (suite[2*i+1] != ' ')
				sprintf(nomnote,"%s/%s_%c%c%d.wav", P->nomInstru[k], P->nomInstru[k] , suite[2*i], suite[2*i+1], j);
			else
				sprintf(nomnote,"%s/%s_%c%d.wav", P->nomInstru[k], P->nomInstru[k], suite[2*i],j);

			N[k][i+1].num[j] = Mix_LoadWAV(nomnote);
			}
		P->tessb[k] = 100*(i+1) + j;//borne inférieure
		if (j > 20)
			{
			P->tessb[k] = -10;
			P->tessh[k] = -10;
			}

		while (N[k][i+1].num[j] != P->err && j < 20/*Sécurité*/)
			{
			i++;
			if (i > 6)
				i = 0;
			if (i == ppte)
				j++;

			if (suite[2*i+1] != ' ')
				sprintf(nomnote,"%s/%s_%c%c%d.wav", P->nomInstru[k], P->nomInstru[k] , suite[2*i], suite[2*i+1], j);
			else
				sprintf(nomnote,"%s/%s_%c%d.wav", P->nomInstru[k], P->nomInstru[k], suite[2*i],j);


			N[k][i+1].num[j] = Mix_LoadWAV(nomnote);
			if (N[k][i+1].num[j] != NULL)
				Mix_VolumeChunk(N[k][i+1].num[j], MIX_MAX_VOLUME*2/3);
			else
				{
				printf ("CACA : %s\n",nomnote);
				N[k][i+1].num[j] = P->err;
				}
			}
		if (j < 20)
			{
			if (i != 0)
				P->tessh[k] = 100*i + j;
			else
				P->tessh[k] = 100*7 + j;
			}
		}//for (k) des instruments

	for (i = 0; i < P->nbreInstru; i++)
		for (l = 1; l < 8; l++)
			for (m = 1; m < 7; m++)
				{
				if (N[i][l].num[m] != NULL)
					P->modele[i][l].num[m] = N[i][l].num[m];
				else
					{
					P->modele[i][l].num[m] = P->err;
					if (i < P->nbreInstru && !(P->tessb[i]%100 < m < P->tessh[i]%100 || (m == P->tessh[i]%100 && l < P->tessh[i]/100) || (m == P->tessb[i]%100 && l > P->tessb[i]/100)))
						{printf ("erreur : P->modele[%d][%d].num[%d]\t%d < %d < %d\n"
									,i, l, m,P->tessb[i], 100*l+m, P->tessh[i]);
						printf ("P->nbreInstru = %d, i = %d\n", P->nbreInstru, i);}
					}
				}//	On met dans P->modele les pointeurs,

/*	m = 0;
	for (i = 0; i < N_I; i++)
		{
		for (j = 0; j < P->attr[i]; j++)
			{
			for (l = 1; l < 8; l++)
				for (k = 1; k < 7; k++)
					P->modele[P->attr2[m]][l].num[k] = P->modele[i]][l].num[k];
			m++;
			}
		}

	for (i = 0; i < N_I; i++)
		if (P->tessb[i] == -10 && P->tessh[i] == -10)//Si une voix a fail...
			{
			for (l = 1; l < 8; l++)
				for (m = 1; m < 7; m++)
					P->modele[P->attr2[i]][l].num[m] = P->modele[0][l].num[m];//... on lui met les pointeurs de la voix 0
			}*/// N'a plus lieu d'être

	for (i = 0; i < P->nbreInstru; i++)
		P->stateVoix[i] = 1;
	for (i = i; i < N_I; i++)
		P->stateVoix[i] = 0;
	return 1;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////




int randmes (mesure* M, mesure *aM, parametres *P, int numVoix, int accForced)// tempo en ms/noire		mesure en tps
{
	int a = 0, b = 0, c = 0, i = 0, max = 0;
	M->nbre[numVoix] = 0;

	for (i = 0; P->bankry[numVoix][i] != -1; i++)
		max++;

	i = 0;
	while (i < 4)//previously while (i != 4)
		{
        b = 0;
		do{
		a = rand()%(max);
		b++;			} while (P->R[P->bankry[numVoix][a]].casse > 4-i);

		for (b = 0; P->R[P->bankry[numVoix][a]].num[b] > 0 && b < 4; b++)		{
			M->r[numVoix][c] = 4*P->mspn/P->R[P->bankry[numVoix][a]].num[b];
			c++;						}

		i += P->R[P->bankry[numVoix][a]].casse;
		}//				initialisation des rythmes de la mesure

	M->nbre[numVoix] = c;
	M->r[numVoix][M->nbre[numVoix]] = -1;

	for (i = 0; i < 8; i ++)
		P->notefreq[i] = P->notefreq[i];

	int tot = P->notefreq[1] + P->notefreq[2] + P->notefreq[3] + P->notefreq[4] + P->notefreq[5] + P->notefreq[6] + P->notefreq[7];
	a = 0;
	b = 0;
// 		 P->bankn contient les numeros des degré le nombre de fois indiqué dans le ccfreq correspondant
//===>
	c = rand()%(tot);

	if (accForced == -1)
		{
		if (P->cadon == 1 && (aM == NULL || aM->cadPos == 0))
			{
			M->cadNum = P->bankcad [rand()%(P->cadfreq[0] + P->cadfreq[1] + P->cadfreq[2] + P->cadfreq[3] + P->cadfreq[4])];// soit une cadence aléatoire parmi P->bankcad
			switch (M->cadNum)
				{
				case CAD_PARFAITE:
					M->cadPos = 1;
					M->acc = 5;		break;
				case CAD_PLAGALE:
					M->cadPos = 1;
					M->acc = 4;		break;
				case CAD_COMPLETE:
					M->cadPos = 3;
					M->acc = 6;		break;
				case CAD_DEUXIEME:
					M->cadPos = 1;
					M->acc = 1;		break;
				case CAD_ACCRAND:
					M->cadPos = 0;		break;
				default:
					M->cadPos = 0;		break;
				}// RAPPEL : cadPos est décroissant !!
			}

		else if (P->cadon == 1 && aM->cadPos != 0)
			{
			M->cadPos = aM->cadPos -1;
			if (M->cadNum == CAD_PARFAITE || M->cadNum == CAD_PLAGALE)
				M->acc = 1;
			else if (M->cadNum == CAD_DEUXIEME)
				M->acc = 2;
			else if (M->cadNum == CAD_COMPLETE)	{
				if (M->cadPos == 2)		M->acc = 4;
				else if (M->cadPos == 1)	M->acc = 5;
				else if (M->cadPos == 0)	M->acc = 1;
				}
			}

		if (P->cadon == 0 || M->cadNum == CAD_ACCRAND)
			{
			M->cadNum = CAD_ACCRAND;
			M->cadPos = 0;
			if (aM != NULL && aM->acc == P->bankn[c])
				M->acc = P->bankn[rand()%(tot)];
			else if (aM != NULL && aM->acc == 5 && rand()%(4) == 0)
				M->acc = 1;
			else
				M->acc = P->bankn[c];// Définition de l'accord aléatoirement
			}
		}// Définition de l'accord
//	else
//		M->acc = accForced; Inutile car M->acc y est déjà : M est la mem


	int notesacc[4];
	notesacc[0] = M->acc;
	notesacc[1] = M->acc + 2;
	if (notesacc[1] >= 8)	notesacc[1] -= 7;
	notesacc[2] = M->acc + 4;
	if (notesacc[2] >= 8)	notesacc[2] -= 7;
	if (M->acc == 5)
		notesacc[3] = 4;
	else
		notesacc[3] = -1;// Initialisation des notes de l'accord dans notesacc

	int dernote = -1;
	if (aM != NULL && aM->n[numVoix][aM->nbre[numVoix]-1] < 1000)
		dernote = aM->n[numVoix][aM->nbre[numVoix]-1];
	else
		dernote = 103;

	if (dernote == 0 || dernote == -1)	{
		dernote = 103;
		printf ("Oh oh\n");		}

	for (i = 0; i < M->nbre[numVoix]; i++)// un int contient une note : d1, num 3 -> 103   ;  d5,n num 4 -> 504, etc
		{
		if (i > 0)
			dernote = M->n[numVoix][i-1];
		M->n[numVoix][i] = 0;
		if (M->acc != 5)	{
			M->n[numVoix][i] += 100*(notesacc[rand()%(3)]);//soit une note alea de l'accord
			a = rand()%(8);
			if (a == 0 && dernote % 100 < P->tessh[P->attr2[numVoix]]%100)	    M->n[numVoix][i] += dernote % 100 +1;
			else if (a == 1 && dernote % 100 > P->tessb[P->attr2[numVoix]]%100) M->n[numVoix][i] += dernote % 100 -1;
			else								M->n[numVoix][i] += (dernote%100);
			if (dernote % 100 < 1)		printf("(CACADERNOTE, %d) ", i);// ==> ddd
			}

		else	{
			M->n[numVoix][i] += 100*(notesacc[rand()%(4)]);//soit une note alea de l'accord + 7e
			a = rand()%(8);
			if (a == 0 && dernote % 100 < P->tessh[P->attr2[numVoix]]%100)	    M->n[numVoix][i] += dernote % 100 + 1;
			else if (a == 1 && dernote % 100 > P->tessb[P->attr2[numVoix]]%100) M->n[numVoix][i] += dernote % 100 - 1;
			else		M->n[numVoix][i] += dernote % 100;
			}

		if (P->paramVoix[numVoix] == 1)
			M->n[numVoix][i] = (M->n[numVoix][i] % 100) + 100 * M->acc;//accord forcé : on oublie le degré et on met
		printf("-%d->%d-(v%d)",dernote, M->n[numVoix][i], numVoix);//		M->acc
		}

	c = 0;
	for (i = 0; i < M->nbre[numVoix]; i++)
		{/*
		while (M->n[numVoix][i]%100 < P->tessb[P->attr2[numVoix]]%100)
			M->n[numVoix][i] += 1;
		while (M->n[numVoix][i]%100 > P->tessh[P->attr2[numVoix]]%100)
			M->n[numVoix][i] -= 1;*/
		if (M->n[numVoix][i]%100 == P->tessb[P->attr2[numVoix]]%100 && M->n[numVoix][i]/100 < P->tessb[P->attr2[numVoix]]/100)
			{
			printf ("ONE");
			M->n[numVoix][i] += 1;
			}
		if (M->n[numVoix][i]%100 == P->tessh[P->attr2[numVoix]]%100 && M->n[numVoix][i]/100 > P->tessh[P->attr2[numVoix]]/100)
			{
			printf ("TWO");
			M->n[numVoix][i] -= 1;
			}

/**/		if (P->modele[P->attr2[numVoix]][M->n[numVoix][i]/100].num[M->n[numVoix][i]%100] == P->err)
			{
			printf("Meow !\tnote = %d\tvoix = %d\ti = %d\n", M->n[numVoix][i], numVoix, i);
			if (M->n[numVoix][i] % 100 >= P->tessh[P->attr2[numVoix]]%100)
				M->n[numVoix][i] -= 1;
			else if (M->n[numVoix][i] % 100 <= P->tessb[P->attr2[numVoix]]%100)
				M->n[numVoix][i] += 1;
			}
/**/		else if (P->modele[P->attr2[numVoix]][M->n[numVoix][i]/100].num[M->n[numVoix][i]%100] == NULL)
			printf("Oops !\tnote = %d\n", M->n[numVoix][i]);

		if (rand()%(5) > 1)
			{
			if (M->n[numVoix][i] == M->n[numVoix][i+2] + 200 && M->n[numVoix][i]+100 < 800)
				{
				M->n[numVoix][i+1] = M->n[numVoix][i]+100;
				printf ("Lisseur runned_a\n");
				}

			else if (M->n[numVoix][i] == M->n[numVoix][i+2] + 201 || M->n[numVoix][i] == M->n[numVoix][i+2] + 201 - 800)
				{
				M->n[numVoix][i+1] = M->n[numVoix][i]+100;
				if (M->n[numVoix][i+1] > 799)
					M->n[numVoix][i]-= (700-1);
				printf ("Lisseur runned_b\n");
				}
			else if (M->n[numVoix][i] == M->n[numVoix][i+2] - 200 && M->n[numVoix][i]-100 > 99)
				{
				M->n[numVoix][i+1] = M->n[numVoix][i]-100;
				printf ("Lisseur runned_c\n");
				}

			else if (M->n[numVoix][i] == M->n[numVoix][i+2] - 201 || M->n[numVoix][i] == M->n[numVoix][i+2] - 201 + 800)
				{
				M->n[numVoix][i+1] = M->n[numVoix][i]-100;
				if (M->n[numVoix][i+1] < 100)
					M->n[numVoix][i] += (700);
				printf ("Lisseur runned_d\n");
				}
			if ( M->n[numVoix][i] >= 800)
				M->n[numVoix][i] -= 700;
			else if ( M->n[numVoix][i+1] >= 800)
				M->n[numVoix][i+1] -= 700;

			if ( M->n[numVoix][i] < 100)
				M->n[numVoix][i] += 700;
			else if ( M->n[numVoix][i+1] < 100)
				M->n[numVoix][i+1] += 700;
			}//if (rand()%(5) !=0)

		}//		"lisseur" : par exemple, do RE mi est dans l'accord de do



	for (i = P->nbreVoix + 1; i < NBRE_VOIX; i++)
		M->nbre[i] = -1;

	for (a = M->nbre[numVoix]; a < 32; a++)
		M->n[numVoix][a] = -1;

	if (accForced == -1)
		{
		for (i = 1; i < P->nbreVoix /*&& i < 4*/; i++)
			{
			randmes (M, aM, P, i, M->acc);
			}
		}
	return 1;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int playclassic (SDL_Surface* fond, SDL_Window* fen, parametres *P)
{
	mesure M, aM;
	SDL_Event event;
	char suite[25], nomnote[20];
	int i = 0, j = 0, k = 0, l[NBRE_VOIX], m[NBRE_VOIX], tps[NBRE_VOIX], state[NBRE_VOIX], comptmes = 1, continuer = 1;
	note chrom[13];//Mix_FreeChunk(son);
//______________________________________________________________________________________________________________________
	SDL_Rect coordrandmus;
	SDL_Color bleu = {10, 10, 200}, orange = {175, 90,10}, noir = {0,0,0};
	SDL_Surface *randmus;
	TTF_Font* police, *police2;
	police = TTF_OpenFont("Police.TTF",60);
	police2 = TTF_OpenFont ("Police.TTF",40);
	bouton bquitter;

	bquitter.cadre = TTF_RenderText_Shaded(police2,"Sortir",noir, bleu);
	bquitter.coord.x = 700 - bquitter.cadre->w - 10;
	bquitter.coord.y = 10;

	coordrandmus.x = 30;
	coordrandmus.y = 15;

	SDL_FillRect(fond,NULL,SDL_MapRGB(fond->format, 10, 10, 200));
	randmus = TTF_RenderText_Shaded(police," RandMus ",bleu, orange);
	SDL_BlitSurface (randmus, NULL, fond, &coordrandmus);
	SDL_BlitSurface (bquitter.cadre, NULL, fond, &bquitter.coord);

	SDL_UpdateWindowSurface (fen);
//______________________________________________________________________________________________________________________

	while (continuer && comptmes < 12000000)
		{
		if (P->tonalite != 12 && P->mode != 3)
			{
			if (comptmes == 1)	randmes (&M, NULL, P, 0, -1);
			else			randmes (&M, &aM,  P, 0, -1);
			printf("a");

			for (j = 0; j < P->nbreVoix; j++)
				{
				state[j] = 0;
				tps[j] = M.r[j][state[j]];
				l[j] = M.n[j][state[j]] / 100;
				m[j] = M.n[j][state[j]] % 100;
				if (M.n[j][state[j]] != -1 && j <= P->nbreVoix && j < NBRE_VOIX)
/**/					if (Mix_PlayChannelTimed(j, P->modele[P->attr2[j]][l[j]].num[m[j]], 0, tps[j]) == -1)	{
						printf(" avec  : note=%d, j = %d et state[j] = %d\n et l=%d et m=%d\n",
								 M.n[j][state[j]], j, state[j], l[state[j]], m[state[j]]);
						printf("Mix_PlayChannelTimed : %s\n",Mix_GetError());
						continuer = 0;
						}
				}

			for (i = P->nbreVoix + 1; i < NBRE_VOIX; i++)
				tps[i] = 4*P->mspn;
			for (i = 0; (state[0] < M.nbre[0] || state[1] < M.nbre[1] || state[2] < M.nbre[2] || state[3] < M.nbre[3]) && i< 16 && continuer; i++)
				{
				for (j = 0; j < P->nbreVoix && j < NBRE_VOIX; j++)
					if (tps[j] == 0 && state[j] < M.nbre[j])
						{
						tps[j] = M.r[j][state[j]];
						l[j] = M.n[j][state[j]] / 100;
						m[j] = M.n[j][state[j]] % 100;
/**/						if (Mix_PlayChannelTimed(j,P->modele[P->attr2[j]][l[j]].num[m[j]], 0, tps[j]) == -1){
							printf("Mix_PlayChannelTimed : %s, v%d\n",Mix_GetError(), j);
							printf("\navec  : note à jouer : %d et state[j] = %d, M.nbre[j] = %d\n", M.n[j][state[j]], state[j], M.nbre[j]);
							Mix_PlayChannelTimed(j,P->err, 0, tps[j]);
						//continuer = 0;
									}
						}
				k = tps[0];
				for (j = 0; j < P->nbreVoix; j++)
					if (tps[j] < k)
						k = tps[j];


				SDL_Delay(k);
				for (j = 0; j < P->nbreVoix; j++)
					{
					tps[j] -= k;
					if (tps[j] == 0)			{
						state[j]++;
						printf ("Ok, v%d\n", j);	}
					else if (tps[j] < 0 && tps[j] > -50)
						{
						tps[j] = 0;
						state[j]++;
						printf("\t bite");
						}
					else if (tps[j] > 0)
						printf ("Ok>, v%d\n", j);
					else
						printf ("Oh oh... v%d in randmes**\n", j);
					}

				while (SDL_PollEvent(&event))
					{
					switch (event.type)
						{
						case SDL_QUIT:
							exit (0);
							break;
						case SDL_MOUSEBUTTONUP:
							if (inbout (event, bquitter))
								continuer = 0;
							break;
						}
					}
				}//Du "grand" for (i)
			printf("Comptmes : %d    cadNum : %d, cadPos : %d\tM.acc : %d\n",comptmes, M.cadNum, M.cadPos,M.acc);
			comptmes++;
			aM = M;
			}// Du if (P->mode != 3)

		else
			{
			sprintf(suite,"a a#b c c#d d#e f f#g g#");
			for (i = 0; i < 12; i++)
				{
				for (j = 2; j < 6; j++)
					{
					if (suite[2*i+1] != ' ')
						sprintf(nomnote,"Piano/piano_%c%c%d.wav",suite[2*i], suite[2*i+1], j);
					else
						sprintf(nomnote,"Piano/piano_%c%d.wav",suite[2*i],j);

					chrom[i+1].num[j-1] = Mix_LoadWAV(nomnote);
					if (chrom[i+1].num[j-1] != NULL)
						Mix_VolumeChunk(chrom[i+1].num[j-1], MIX_MAX_VOLUME*2/3);
					else
						printf ("CACA :%s\n",nomnote);
					}
				}

			while (continuer)
				{
				j = rand()%(6) + 1;
				i = rand()%(4) + 1;
				tps[0] = rand()%(2000) + 100;//tps[0] : un int comme un autre...

				Mix_PlayChannelTimed(1, chrom[j].num[i], 1, tps[0]);
				SDL_Delay (tps[0]);

				while (SDL_PollEvent(&event))
					{
					switch (event.type)
						{
						case SDL_QUIT:
							exit (0);
							break;
						case SDL_MOUSEBUTTONUP:
							if (inbout (event, bquitter))
								continuer = 0;
							break;
						}
					}
				}// du while (continuer)
			for (i = 0; i < 13; i++)
				for (j = 0; j < 5; j++)
					{
					Mix_FreeChunk(chrom[i].num[j]);
					}
			}
		}
	return 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////:

int inbout (SDL_Event event, bouton bout)
{

	if (event.type != SDL_MOUSEBUTTONDOWN && event.type != SDL_MOUSEBUTTONUP)
		return 0;

	if (event.button.y > bout.coord.y && event.button.y < bout.coord.y + bout.cadre->h && event.button.x > bout.coord.x && event.button.x < bout.coord.x + bout.cadre->w)
		return 1;
	else
		return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int inbout2 (SDL_Event event, SDL_Rect coord, SDL_Surface *cadre)
{

	if (event.type != SDL_MOUSEBUTTONDOWN && event.type != SDL_MOUSEBUTTONUP)
		return 0;

	if (event.button.y > coord.y && event.button.y < coord.y + cadre->h && event.button.x > coord.x && event.button.x < coord.x + cadre->w)
		return 1;
	else
		return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int flipboutp (boutp* bout)
{
	bout->coordp.y = bout->coord.y-5;
	bout->coordm.y = bout->coord.y-5;
	bout->coordv.y = bout->coord.y;
//	bout->coordp.x = 10 + bout->coord.x + bout->titre->w + bout->valeur->w;
	bout->coordp.x = 500;
	bout->coordm.x = bout->coord.x - bout->moins->w - 5;
	bout->coordv.x = bout->coord.x + bout->titre->w;
	return 1;
}
int inparam (parametres* P, SDL_Surface *fond, SDL_Window* fen)
{
	int continuer = 1, continuer2 = 1, i = 0, j = 0, k = 0, l = 0, m = 0, n = 0, voixAct = 0, gammeAvant = P->tonalite, modeAvant = P->mode;
	k = 0;
	char nom[30];
	const char nomtonalites[] = "La  La# Si  Do  Do# Re  Re# Mi  Fa  Fa# Sol Sol#";
	const char nommodes[] = "Majeur     Mineur     Melodique  Chromatique";
	bouton bquitter, brythmes, bnotes, btonmode, bvoix, bflechep, bflechem, bnVoix, bcad, bcadon, bbassForced;
	bouton bdefVoix[4], battrInstru[P->nbreInstru], bnInstruVoix, bTempop, bTempom, bindic1, bindic2;
	boutp tempo, tons, mode, bpnbreVoix;
	boutp not[8], ryt[8], cad[5], voi, vol, instruVol;
	SDL_Event event;
	TTF_Font* police, *police2, *police3;
	police = TTF_OpenFont("Police.TTF",40);
	police2 = TTF_OpenFont ("Police.TTF",30);
	police3 = TTF_OpenFont ("Police.TTF",23);
	SDL_Color bleu = {10, 10, 200}, bleuf = {0, 0, 60}, orange = {175, 90,10}, noir = {0,0,0};

	bquitter.cadre = TTF_RenderText_Shaded (police, "Sortir", noir, bleu);
	bquitter.coord.x = 700 - bquitter.cadre->w - 10;
	bquitter.coord.y = 10;

	brythmes.cadre = TTF_RenderText_Shaded (police, "Frequences des rythmes", bleu, orange);
	brythmes.coord.x = 20;
	brythmes.coord.y = 70;

	bnotes.cadre = TTF_RenderText_Shaded (police, "Frequences des notes", bleu, orange);
	bnotes.coord.x = 20;
	bnotes.coord.y = 135;

	btonmode.cadre = TTF_RenderText_Shaded (police, "Tonalite et tempo", bleu, orange);
	btonmode.coord.x = 20;
	btonmode.coord.y = 200;

	bvoix.cadre = TTF_RenderText_Shaded (police, "Voix ... ", bleu, orange);
	bvoix.coord.x = 20;
	bvoix.coord.y = 265;

	bcad.cadre = TTF_RenderText_Shaded (police, "Cadences", bleu, orange);
	bcad.coord.x = 20;
	bcad.coord.y = 330;

	SDL_FillRect(fond,NULL,SDL_MapRGB(fond->format, 10, 10, 200));
	SDL_BlitSurface (bquitter.cadre, NULL, fond, &bquitter.coord);
	SDL_BlitSurface (brythmes.cadre, NULL, fond, &brythmes.coord);
	SDL_BlitSurface (btonmode.cadre, NULL, fond, &btonmode.coord);
	SDL_BlitSurface (bnotes.cadre, NULL, fond, &bnotes.coord);
	SDL_BlitSurface (bcad.cadre, NULL, fond, &bcad.coord);
	SDL_BlitSurface (bvoix.cadre, NULL, fond, &bvoix.coord);
	SDL_UpdateWindowSurface (fen);

	while (continuer)
		{
		SDL_WaitEvent (&event);
		switch (event.type)
			{
			case SDL_QUIT:
				exit (0);
				break;
			case SDL_MOUSEBUTTONUP:
				if (inbout (event, bquitter))
					continuer = 0;
				else if (inbout (event, bvoix))
//<=<=<=<=<=<=<=<=<=<=<=<=<=<=<=<=<=<=<<= Pour lisibilité
{
SDL_FillRect(fond,NULL,SDL_MapRGB(fond->format, 10, 10, 200));
SDL_BlitSurface (bquitter.cadre, NULL, fond, &bquitter.coord);

m = 0;
l = 0;

for (i = 0; i < P->nbreInstru; i++)
	{
	sprintf (nom, "-> %s", P->nomInstru[i]);
	battrInstru[i].cadre = TTF_RenderText_Shaded (police3,nom , bleu, orange);
	battrInstru[i].coord.x = 550;
	battrInstru[i].coord.y = 85 + 35*i;
	SDL_BlitSurface (battrInstru[i].cadre, NULL, fond, &battrInstru[i].coord);
	}

bpnbreVoix.titre = TTF_RenderText_Shaded (police, "Nombre de voix : ", orange, bleu);
bpnbreVoix.plus = TTF_RenderText_Shaded (police, "+", bleu, orange);
bpnbreVoix.moins = TTF_RenderText_Shaded (police, "-", bleu, orange);
sprintf(nom, "%d", P->nbreVoix);
bpnbreVoix.valeur = TTF_RenderText_Shaded (police,nom , orange, bleu);
bpnbreVoix.coord.x = 70;
bpnbreVoix.coord.y = 65;
flipboutp(&bpnbreVoix);

voi.titre = TTF_RenderText_Shaded (police, "Voix : ", orange, bleu);
voi.plus = TTF_RenderText_Shaded (police, "+", bleu, orange);
voi.moins = TTF_RenderText_Shaded (police, "-", bleu, orange);
sprintf(nom, "%d", m+1);
voi.valeur = TTF_RenderText_Shaded (police,nom , orange, bleu);
voi.coord.x = 70;
voi.coord.y = 140;
flipboutp(&voi);

vol.titre = TTF_RenderText_Shaded (police, "Volume : ", orange, bleu);
vol.plus = TTF_RenderText_Shaded (police, "+", bleu, orange);
vol.moins = TTF_RenderText_Shaded (police, "-", bleu, orange);
sprintf (nom, "%d", P->volInstru[n]);
vol.valeur = TTF_RenderText_Shaded (police,nom , orange, bleu);
vol.coord.x = 70;
vol.coord.y = 350;
flipboutp(&vol);
vol.coordp.x -= 50;

instruVol.titre = TTF_RenderText_Shaded (police, "Voix de ", orange, bleu);
instruVol.plus = TTF_RenderText_Shaded (police, "+", bleu, orange);
instruVol.moins = TTF_RenderText_Shaded (police, "-", bleu, orange);
sprintf (nom, "%s", P->nomInstru[n]);
instruVol.valeur = TTF_RenderText_Shaded (police,nom , orange, bleu);
instruVol.coord.x = 70;
instruVol.coord.y = 285;
flipboutp(&instruVol);
instruVol.coordp.x -= 50;

bindic1.cadre = TTF_RenderText_Shaded (police, "Attribution des voix", bleuf ,bleu);
bindic1.coord.x = 70;
bindic1.coord.y = 5;

bindic2.cadre = TTF_RenderText_Shaded (police, "Volume des voix", bleuf ,bleu);
bindic2.coord.x = 70;
bindic2.coord.y = 220;

sprintf (nom, "%s", P->nomInstru[P->attr2[m]]);
bnInstruVoix.cadre = TTF_RenderText_Shaded (police2, nom, orange, bleu);
bnInstruVoix.coord.x = voi.coord.x+270;
bnInstruVoix.coord.y = 148;

SDL_BlitSurface (bnInstruVoix.cadre, NULL, fond, &bnInstruVoix.coord);
SDL_BlitSurface (bpnbreVoix.titre, NULL, fond, &bpnbreVoix.coord);
SDL_BlitSurface (bpnbreVoix.plus, NULL, fond, &bpnbreVoix.coordp);
SDL_BlitSurface (bpnbreVoix.moins, NULL, fond, &bpnbreVoix.coordm);
SDL_BlitSurface (bpnbreVoix.valeur, NULL, fond, &bpnbreVoix.coordv);
SDL_BlitSurface (voi.titre, NULL, fond, &voi.coord);
SDL_BlitSurface (voi.plus, NULL, fond, &voi.coordp);
SDL_BlitSurface (voi.moins, NULL, fond, &voi.coordm);
SDL_BlitSurface (voi.valeur, NULL, fond, &voi.coordv);
SDL_BlitSurface (vol.titre, NULL, fond, &vol.coord);
SDL_BlitSurface (vol.plus, NULL, fond, &vol.coordp);
SDL_BlitSurface (vol.moins, NULL, fond, &vol.coordm);
SDL_BlitSurface (vol.valeur, NULL, fond, &vol.coordv);
SDL_BlitSurface (instruVol.titre, NULL, fond, &instruVol.coord);
SDL_BlitSurface (instruVol.plus, NULL, fond, &instruVol.coordp);
SDL_BlitSurface (instruVol.moins, NULL, fond, &instruVol.coordm);
SDL_BlitSurface (instruVol.valeur, NULL, fond, &instruVol.coordv);
SDL_BlitSurface (bindic1.cadre, NULL, fond, &bindic1.coord);
SDL_BlitSurface (bindic2.cadre, NULL, fond, &bindic2.coord);

SDL_UpdateWindowSurface (fen);

continuer2 = 1;
while (continuer2)
	{
	SDL_WaitEvent (&event);
	switch (event.type)
		{
		case SDL_QUIT:
			exit (0);
			break;
		case SDL_MOUSEBUTTONUP:
			if (inbout2 (event, bpnbreVoix.coordp, bpnbreVoix.plus) && P->nbreVoix < NBRE_VOIX -1)
				{//Bouton nombre de voix+ : on ajoute 1 et refait l'affichage
				sprintf (nom, "%d", P->nbreVoix);
				bpnbreVoix.valeur = TTF_RenderText_Shaded (police,nom, bleu, bleu);
				SDL_BlitSurface (bpnbreVoix.valeur, NULL, fond, &bpnbreVoix.coordv);
				P->nbreVoix++;
				sprintf (nom, "%d", P->nbreVoix);
				bpnbreVoix.valeur = TTF_RenderText_Shaded (police,nom, orange, bleu);
				SDL_BlitSurface (bpnbreVoix.valeur, NULL, fond, &bpnbreVoix.coordv);
				SDL_UpdateWindowSurface (fen);
				}
			else if (inbout2 (event, bpnbreVoix.coordm, bpnbreVoix.moins) && P->nbreVoix > 0)
				{//bouton nbre de voix- : on retire 1, refait l'affichage
				sprintf (nom, "%d", P->nbreVoix);
				bpnbreVoix.valeur = TTF_RenderText_Shaded (police,nom, bleu, bleu);
				SDL_BlitSurface (bpnbreVoix.valeur, NULL, fond, &bpnbreVoix.coordv);
				P->nbreVoix--;
				sprintf (nom, "%d", P->nbreVoix);
				bpnbreVoix.valeur = TTF_RenderText_Shaded (police,nom, orange, bleu);
				SDL_BlitSurface (bpnbreVoix.valeur, NULL, fond, &bpnbreVoix.coordv);
				SDL_UpdateWindowSurface (fen);
				}
			else if (inbout2 (event, voi.coordp, voi.plus))
				{
				sprintf (nom, "%d", m+1);
				voi.valeur = TTF_RenderText_Shaded (police,nom, bleu, bleu);
				SDL_BlitSurface (voi.valeur, NULL, fond, &voi.coordv);
				sprintf (nom, "%s", P->nomInstru[P->attr2[m]]);
				bnInstruVoix.cadre = TTF_RenderText_Shaded (police2, nom, bleu, bleu);
				SDL_BlitSurface (bnInstruVoix.cadre, NULL, fond, &bnInstruVoix.coord);
				m++;
				if (m >= N_I)
					m = 0;
				sprintf (nom, "%d", m+1);
				voi.valeur = TTF_RenderText_Shaded (police,nom, orange, bleu);
				sprintf (nom, "%s", P->nomInstru[P->attr2[m]]);
				bnInstruVoix.cadre = TTF_RenderText_Shaded (police2, nom, orange, bleu);
				SDL_BlitSurface (bnInstruVoix.cadre, NULL, fond, &bnInstruVoix.coord);
				SDL_BlitSurface (voi.valeur, NULL, fond, &voi.coordv);
				SDL_UpdateWindowSurface (fen);
				}
			else if (inbout2 (event, voi.coordm, voi.moins))
				{
				sprintf (nom, "%d", m+1);
				voi.valeur = TTF_RenderText_Shaded (police,nom, bleu, bleu);
				SDL_BlitSurface (voi.valeur, NULL, fond, &voi.coordv);
				sprintf (nom, "%s", P->nomInstru[P->attr2[m]]);
				bnInstruVoix.cadre = TTF_RenderText_Shaded (police2, nom, bleu, bleu);
				SDL_BlitSurface (bnInstruVoix.cadre, NULL, fond, &bnInstruVoix.coord);
				m--;
				if (m <= -1)
					m = N_I - 1;
				sprintf (nom, "%d", m+1);
				voi.valeur = TTF_RenderText_Shaded (police,nom, orange, bleu);
				SDL_BlitSurface (bnInstruVoix.cadre, NULL, fond, &bnInstruVoix.coord);
				sprintf (nom, "%s", P->nomInstru[P->attr2[m]]);
				bnInstruVoix.cadre = TTF_RenderText_Shaded (police2, nom, orange, bleu);
				SDL_BlitSurface (voi.valeur, NULL, fond, &voi.coordv);
				SDL_BlitSurface (bnInstruVoix.cadre, NULL, fond, &bnInstruVoix.coord);
				SDL_UpdateWindowSurface (fen);
				}

			else if (inbout2 (event, instruVol.coordp, instruVol.plus))
				{
				sprintf (nom, "%s",P->nomInstru[n]);
				instruVol.valeur = TTF_RenderText_Shaded (police,nom, bleu, bleu);
				SDL_BlitSurface (instruVol.valeur, NULL, fond, &instruVol.coordv);
				sprintf (nom, "%d", P->volInstru[n]);
				vol.valeur = TTF_RenderText_Shaded (police,nom, bleu, bleu);
				SDL_BlitSurface (vol.valeur, NULL, fond, &vol.coordv);
				n++;
				if (n >= P->nbreInstru)
					n = 0;
				sprintf (nom, "%s", P->nomInstru[n]);
				instruVol.valeur = TTF_RenderText_Shaded (police,nom, orange, bleu);
				SDL_BlitSurface (instruVol.valeur, NULL, fond, &instruVol.coordv);
				sprintf (nom, "%d", P->volInstru[n]);
				vol.valeur = TTF_RenderText_Shaded (police,nom, orange, bleu);
				SDL_BlitSurface (vol.valeur, NULL, fond, &vol.coordv);
				SDL_UpdateWindowSurface (fen);
				}
			else if (inbout2 (event, instruVol.coordm, instruVol.moins))
				{
				sprintf (nom, "%s",P->nomInstru[n]);
				instruVol.valeur = TTF_RenderText_Shaded (police,nom, bleu, bleu);
				SDL_BlitSurface (instruVol.valeur, NULL, fond, &instruVol.coordv);
				sprintf (nom, "%d", P->volInstru[n]);
				vol.valeur = TTF_RenderText_Shaded (police,nom, bleu, bleu);
				SDL_BlitSurface (vol.valeur, NULL, fond, &vol.coordv);
				n--;
				if (n <= -1)
					n = P->nbreInstru-1;
				sprintf (nom, "%s", P->nomInstru[n]);
				instruVol.valeur = TTF_RenderText_Shaded (police,nom, orange, bleu);
				SDL_BlitSurface (instruVol.valeur, NULL, fond, &instruVol.coordv);
				sprintf (nom, "%d", P->volInstru[n]);
				vol.valeur = TTF_RenderText_Shaded (police,nom, orange, bleu);
				SDL_BlitSurface (vol.valeur, NULL, fond, &vol.coordv);
				SDL_UpdateWindowSurface (fen);
				}

			else if (inbout2 (event, vol.coordp, vol.plus))
				{
				sprintf (nom, "%d", P->volInstru[n]);
				vol.valeur = TTF_RenderText_Shaded (police,nom, bleu, bleu);
				SDL_BlitSurface (vol.valeur, NULL, fond, &vol.coordv);
				if ( P->volInstru[n] < 100)
					P->volInstru[n] += 5;
				sprintf (nom, "%d", P->volInstru[n]);
				vol.valeur = TTF_RenderText_Shaded (police,nom, orange, bleu);
				SDL_BlitSurface (vol.valeur, NULL, fond, &vol.coordv);
				SDL_UpdateWindowSurface (fen);
				}
			else if (inbout2 (event, vol.coordm, vol.moins))
				{
				sprintf (nom, "%d", P->volInstru[n]);
				vol.valeur = TTF_RenderText_Shaded (police,nom, bleu, bleu);
				SDL_BlitSurface (vol.valeur, NULL, fond, &vol.coordv);
				if ( P->volInstru[n] > 10)
					P->volInstru[n] -= 5;
				sprintf (nom, "%d", P->volInstru[n]);
				vol.valeur = TTF_RenderText_Shaded (police,nom, orange, bleu);
				SDL_BlitSurface (vol.valeur, NULL, fond, &vol.coordv);
				SDL_UpdateWindowSurface (fen);
				}

			else if (inbout (event, bquitter))
				{
				SDL_FillRect(fond,NULL,SDL_MapRGB(fond->format, 10, 10, 200));
				SDL_BlitSurface (bquitter.cadre, NULL, fond, &bquitter.coord);
				SDL_BlitSurface (brythmes.cadre, NULL, fond, &brythmes.coord);
				SDL_BlitSurface (btonmode.cadre, NULL, fond, &btonmode.coord);
				SDL_BlitSurface (bnotes.cadre, NULL, fond, &bnotes.coord);
				SDL_BlitSurface (bcad.cadre, NULL, fond, &bcad.coord);
				SDL_BlitSurface (bvoix.cadre, NULL, fond, &bvoix.coord);
				SDL_UpdateWindowSurface (fen);
				continuer2 = 0; continuer = 1;// Reblit' du menu paramètres
				}
			for (i = 0; i < P->nbreInstru; i++)
				if (inbout (event, battrInstru[i]))
					{
					sprintf (nom, "%s", P->nomInstru[P->attr2[m]]);
					bnInstruVoix.cadre = TTF_RenderText_Shaded (police2, nom, bleu, bleu);
					SDL_BlitSurface (bnInstruVoix.cadre, NULL, fond, &bnInstruVoix.coord);
					P->attr2[m] = i;// --> suffisant =)
					sprintf (nom, "%s", P->nomInstru[P->attr2[m]]);
					bnInstruVoix.cadre = TTF_RenderText_Shaded (police2, nom, orange, bleu);
					SDL_BlitSurface (bnInstruVoix.cadre, NULL, fond, &bnInstruVoix.coord);
					SDL_UpdateWindowSurface (fen);
					}

			break;
		}
	}

}
//<=<=<=<=<=<=<=<=<=<=<=<=<=<=<=<=<=<=<<= Pour lisibilité
				else if (inbout (event, bcad))
//<=<=<=<=<=<=<=<=<=<=<=<=<=<=<=<=<=<=<<= Pour lisibilité
{
SDL_FillRect(fond,NULL,SDL_MapRGB(fond->format, 10, 10, 200));
SDL_BlitSurface (bquitter.cadre, NULL, fond, &bquitter.coord);

cad[0].titre = TTF_RenderText_Shaded (police2,"Frequence de la cadence 5,1 : ", orange, bleu);
cad[1].titre = TTF_RenderText_Shaded (police2,"Frequence de la cadence 4, 1 : ", orange, bleu);
cad[2].titre = TTF_RenderText_Shaded (police2,"Frequence de la cadence 6, 4, 5, 1 : ", orange, bleu);
cad[3].titre = TTF_RenderText_Shaded (police2,"Frequence de la cadence 1, 2 : ", orange, bleu);
cad[4].titre = TTF_RenderText_Shaded (police2,"Frequence des accords aleatoires : ", orange, bleu);

if (P->cadon == 1)
	bcadon.cadre = TTF_RenderText_Shaded (police2,"Cadences : activees", orange, bleu);
else
	bcadon.cadre = TTF_RenderText_Shaded (police2,"Cadences : desactivees", orange, bleu);
bcadon.coord.x = 20;
bcadon.coord.y = 20;

SDL_BlitSurface(bcadon.cadre, NULL, fond, &bcadon.coord);

for (i = 0; i < 5; i++)
	{
	cad[i].coord.x = 50;
	cad[i].coord.y = 85 + 55*i;
	cad[i].plus = TTF_RenderText_Shaded (police,"+", bleu, orange);
	cad[i].moins = TTF_RenderText_Shaded (police,"-", bleu, orange);

	sprintf (nom, "%d", P-> cadfreq[i]);
	cad[i].valeur = TTF_RenderText_Shaded (police2, nom, orange, bleu);

	flipboutp (&cad[i]);
	cad[i].coordp.x = 600;
	cad[i].coordv.x = 550;

	SDL_BlitSurface(cad[i].titre, NULL, fond, &cad[i].coord);
	SDL_BlitSurface(cad[i].plus, NULL, fond, &cad[i].coordp);
	SDL_BlitSurface(cad[i].moins, NULL, fond, &cad[i].coordm);
	SDL_BlitSurface(cad[i].valeur, NULL, fond, &cad[i].coordv);
	}

SDL_UpdateWindowSurface (fen);

continuer2 = 1;
while (continuer2)
	{
	SDL_WaitEvent (&event);
	switch (event.type)
		{
		case SDL_QUIT:
			exit(0);
			break;
		case SDL_MOUSEBUTTONUP:
			for (i = 0; i < 5; i++)
				if (inbout2 (event, cad[i].coordp, cad[i].plus) && P->cadfreq[i] < 100)
					{
					sprintf (nom, "%d", P->cadfreq[i]);
					cad[i].valeur = TTF_RenderText_Shaded (police2, nom, bleu, bleu);
					SDL_BlitSurface(cad[i].valeur, NULL, fond, &cad[i].coordv);
					P->cadfreq[i]++;
					sprintf (nom, "%d", P->cadfreq[i]);
					cad[i].valeur = TTF_RenderText_Shaded (police2, nom, orange, bleu);
					SDL_BlitSurface(cad[i].valeur, NULL, fond, &cad[i].coordv);
					SDL_UpdateWindowSurface (fen);
					}

				else if (inbout2 (event, cad[i].coordm, cad[i].moins) && P->cadfreq[i] > 0
		&& P->cadfreq[0] + P->cadfreq[1] + P->cadfreq[2] + P->cadfreq[3] + P->cadfreq[4] > 1)
					{
					sprintf (nom, "%d", P->cadfreq[i]);
					cad[i].valeur = TTF_RenderText_Shaded (police2, nom, bleu, bleu);
					SDL_BlitSurface(cad[i].valeur, NULL, fond, &cad[i].coordv);
					P->cadfreq[i]--;
					sprintf (nom, "%d", P->cadfreq[i]);
					cad[i].valeur = TTF_RenderText_Shaded (police2, nom, orange, bleu);
					SDL_BlitSurface(cad[i].valeur, NULL, fond, &cad[i].coordv);
					SDL_UpdateWindowSurface (fen);
					}
			if (inbout (event, bcadon))
				{
				if (P->cadon == 0)
					{
					bcadon.cadre = TTF_RenderText_Shaded (police2,"Cadences : desactivees", bleu, bleu);
					SDL_BlitSurface(bcadon.cadre, NULL, fond, &bcadon.coord);
					P->cadon = 1;
					bcadon.cadre = TTF_RenderText_Shaded (police2,"Cadences : activees", orange, bleu);
					SDL_BlitSurface(bcadon.cadre, NULL, fond, &bcadon.coord);
					SDL_UpdateWindowSurface (fen);
					}
				else
					{
					bcadon.cadre = TTF_RenderText_Shaded (police2,"Cadences : activees", bleu, bleu);
					SDL_BlitSurface(bcadon.cadre, NULL, fond, &bcadon.coord);
					P->cadon = 0;
					bcadon.cadre = TTF_RenderText_Shaded (police2,"Cadences : desactivees", orange, bleu);
					SDL_BlitSurface(bcadon.cadre, NULL, fond, &bcadon.coord);
					SDL_UpdateWindowSurface (fen);
					}
				}
			else if (inbout (event, bquitter))
				{
				SDL_FillRect(fond,NULL,SDL_MapRGB(fond->format, 10, 10, 200));
				SDL_BlitSurface (bquitter.cadre, NULL, fond, &bquitter.coord);
				SDL_BlitSurface (brythmes.cadre, NULL, fond, &brythmes.coord);
				SDL_BlitSurface (btonmode.cadre, NULL, fond, &btonmode.coord);
				SDL_BlitSurface (bnotes.cadre, NULL, fond, &bnotes.coord);
				SDL_BlitSurface (bcad.cadre, NULL, fond, &bcad.coord);
				SDL_BlitSurface (bvoix.cadre, NULL, fond, &bvoix.coord);
				SDL_UpdateWindowSurface (fen);
				continuer2 = 0;// Reblit' du menu paramètres
				}

			break;
		}
	}
}
//<=<=<=<=<=<=<=<=<=<=<=<=<=<=<=<=<=<=<<= Pour lisibilité
				else if (inbout (event, brythmes))
//<=<=<=<=<=<=<=<=<=<=<=<=<=<=<=<=<=<=<<= Pour lisibilité
{
SDL_FillRect(fond,NULL,SDL_MapRGB(fond->format, 10, 10, 200));
SDL_BlitSurface (bquitter.cadre, NULL, fond, &bquitter.coord);

if (P->paramVoix[voixAct] == 0)
	sprintf(nom, "Fondamentale forcee : non");
else
	sprintf(nom, "Fondamentale forcee : oui");
bbassForced.cadre = TTF_RenderText_Shaded(police3, nom, orange, bleu);
bbassForced.coord.x = 275;
bbassForced.coord.y = 20;

sprintf(nom, "<==");
bflechem.coord.y = 10;
bflechem.coord.x = 20;
bflechem.cadre = TTF_RenderText_Shaded(police2, nom, orange, bleu);

sprintf (nom, "Voix : %d", voixAct+1);
bnVoix.coord.y = 10;
bnVoix.coord.x = bflechem.coord.x + bflechem.cadre->w +15;
bnVoix.cadre = TTF_RenderText_Shaded(police2, nom, orange, bleu);

sprintf(nom, "==>");
bflechep.coord.y = 10;
bflechep.coord.x = bnVoix.coord.x + bnVoix.cadre->w + 25;
bflechep.cadre = TTF_RenderText_Shaded(police2, nom, orange, bleu);

SDL_BlitSurface (bnVoix.cadre, NULL, fond, &bnVoix.coord);
SDL_BlitSurface (bflechep.cadre, NULL, fond, &bflechep.coord);
SDL_BlitSurface (bflechem.cadre, NULL, fond, &bflechem.coord);

sprintf (nom, "Frequence de la ronde : ");
ryt[0].titre = TTF_RenderText_Shaded (police2, nom, orange, bleu);
sprintf (nom, "Frequence de la blanche : ");
ryt[1].titre = TTF_RenderText_Shaded (police2, nom, orange, bleu);
sprintf (nom, "Frequence de la noire : ");
ryt[2].titre = TTF_RenderText_Shaded (police2, nom, orange, bleu);
sprintf (nom, "Frequence des croches : ");
ryt[3].titre = TTF_RenderText_Shaded (police2, nom, orange, bleu);
sprintf (nom, "Frequence de la saute : ");
ryt[4].titre = TTF_RenderText_Shaded (police2, nom, orange, bleu);
sprintf (nom, "Frequence des doubles : ");
ryt[5].titre = TTF_RenderText_Shaded (police2, nom, orange, bleu);

for (i = 0; i < 4; i++)
	{
	sprintf (nom, "Def : V%d", i+1);
	bdefVoix[i].cadre = TTF_RenderText_Shaded (police2, nom, orange, bleu);
	bdefVoix[i].coord.x = 570;
	bdefVoix[i].coord.y = 120 + 45*i;
	SDL_BlitSurface (bdefVoix[i].cadre, NULL, fond, &bdefVoix[i].coord);
	}

for (i = 0; i < 6; i++)
	{
	sprintf (nom, "%d", P->rythfreq[0][i]);
	ryt[i].valeur = TTF_RenderText_Shaded (police2, nom, orange, bleu);

	ryt[i].plus = TTF_RenderText_Shaded (police,"+", bleu, orange);
	ryt[i].moins = TTF_RenderText_Shaded (police,"-", bleu, orange);
	ryt[i].coord.x = 50;
	ryt[i].coord.y = 85 +55*i;
	flipboutp(&ryt[i]);

	SDL_BlitSurface(ryt[i].titre, NULL, fond, &ryt[i].coord);
	SDL_BlitSurface(ryt[i].plus, NULL, fond, &ryt[i].coordp);
	SDL_BlitSurface(ryt[i].moins, NULL, fond, &ryt[i].coordm);
	SDL_BlitSurface(ryt[i].valeur, NULL, fond, &ryt[i].coordv);
	}
SDL_BlitSurface (bbassForced.cadre, NULL, fond, &bbassForced.coord);
SDL_UpdateWindowSurface (fen);

continuer2 = 1;
while (continuer2)
	{
	SDL_WaitEvent (&event);
	switch (event.type)
		{
		case SDL_QUIT:
			exit (0);
		case SDL_MOUSEBUTTONUP:
			for (i = 0; i < 6; i++)
				{
				if (inbout2 (event, ryt[i].coordp, ryt[i].plus) && P->rythfreq[voixAct][i] < 100)
					{
					sprintf (nom, "%d", P->rythfreq[voixAct][i]);
					ryt[i].valeur = TTF_RenderText_Shaded (police2,nom, bleu, bleu);
					SDL_BlitSurface (ryt[i].valeur, NULL, fond, &ryt[i].coordv);
					P->rythfreq[voixAct][i]++;
					sprintf (nom, "%d", P->rythfreq[voixAct][i]);
					ryt[i].valeur = TTF_RenderText_Shaded (police2,nom, orange, bleu);
					SDL_BlitSurface (ryt[i].valeur, NULL, fond, &ryt[i].coordv);
					SDL_UpdateWindowSurface (fen);
					}
				else if (inbout2 (event, ryt[i].coordm, ryt[i].moins) && P->rythfreq[voixAct][i] > 0
	&& P->rythfreq[voixAct][0] + P->rythfreq[voixAct][1] + P->rythfreq[voixAct][2] + P->rythfreq[voixAct][3] + P->rythfreq[voixAct][4] + P->rythfreq[voixAct][5] + P->rythfreq[voixAct][6] >0)
					{
					sprintf (nom, "%d", P->rythfreq[voixAct][i]);
					ryt[i].valeur = TTF_RenderText_Shaded (police2,nom, bleu, bleu);
					SDL_BlitSurface (ryt[i].valeur, NULL, fond, &ryt[i].coordv);
					P->rythfreq[voixAct][i]--;
					sprintf (nom, "%d", P->rythfreq[voixAct][i]);
					ryt[i].valeur = TTF_RenderText_Shaded (police2,nom, orange, bleu);
					SDL_BlitSurface (ryt[i].valeur, NULL, fond, &ryt[i].coordv);
					SDL_UpdateWindowSurface (fen);
					}
				}
			for (i = 0; i < 4; i++)
				if (inbout (event, bdefVoix[i]))
					{
					for (j = 0; j < 6; j++)
						{
						sprintf (nom, "%d", P->rythfreq[voixAct][j]);
						ryt[j].valeur = TTF_RenderText_Shaded (police2,nom, bleu, bleu);
						SDL_BlitSurface (ryt[j].valeur, NULL, fond, &ryt[j].coordv);
						}// on efface les valeurs actuellement affichées

					for (j = 0; j < 6; j++)
						P->rythfreq[voixAct][j] = P->rythfreq[NBRE_VOIX + i][j];
						//on initialise les valeurs à celles pas défaut

					for (j = 0; j < 6; j++)
						{
						sprintf (nom, "%d", P->rythfreq[voixAct][j]);
						ryt[j].valeur = TTF_RenderText_Shaded (police2,nom, orange, bleu);
						SDL_BlitSurface (ryt[j].valeur, NULL, fond, &ryt[j].coordv);
						}//On affiche les nouvelles valeurs
					SDL_UpdateWindowSurface (fen);
					}

			if (inbout (event, bflechep) && voixAct < NBRE_VOIX-1)
				{// On passe de la voix n à la voix n+1
				for (i = 0; i < 6; i++)
					{
					sprintf (nom, "%d", P->rythfreq[voixAct][i]);
					ryt[i].valeur = TTF_RenderText_Shaded (police2, nom, bleu, bleu);

					SDL_BlitSurface(ryt[i].valeur, NULL, fond, &ryt[i].coordv);
					}
				sprintf (nom, "Voix : %d", voixAct+1);
				bnVoix.cadre = TTF_RenderText_Shaded (police2, nom, bleu, bleu);
				if (P->paramVoix[voixAct] == 0)
					sprintf(nom, "Fondamentale forcee : non");
				else
					sprintf(nom, "Fondamentale forcee : oui");
				bbassForced.cadre = TTF_RenderText_Shaded(police3, nom, bleu, bleu);
				SDL_BlitSurface (bbassForced.cadre, NULL, fond, &bbassForced.coord);
				SDL_BlitSurface(bnVoix.cadre, NULL, fond, &bnVoix.coord);

				voixAct++;

				for (i = 0; i < 6; i++)
					{
					sprintf (nom, "%d", P->rythfreq[voixAct][i]);
					ryt[i].valeur = TTF_RenderText_Shaded (police2, nom, orange, bleu);

					SDL_BlitSurface(ryt[i].valeur, NULL, fond, &ryt[i].coordv);
					}
				sprintf (nom, "Voix : %d", voixAct+1);
				bnVoix.cadre = TTF_RenderText_Shaded (police2, nom, orange, bleu);
				if (P->paramVoix[voixAct] == 0)
					sprintf(nom, "Fondamentale forcee : non");
				else
					sprintf(nom, "Fondamentale forcee : oui");
				bbassForced.cadre = TTF_RenderText_Shaded(police3, nom, orange, bleu);
				SDL_BlitSurface (bbassForced.cadre, NULL, fond, &bbassForced.coord);
				SDL_BlitSurface(bnVoix.cadre, NULL, fond, &bnVoix.coord);
				SDL_UpdateWindowSurface (fen);
				}//du inbout bflechep

			else if (inbout (event, bflechem) && voixAct > 0)
				{// On passe de la voix n à la voix n-1
				for (i = 0; i < 6; i++)
					{
					sprintf (nom, "%d", P->rythfreq[voixAct][i]);
					ryt[i].valeur = TTF_RenderText_Shaded (police2, nom, bleu, bleu);

					SDL_BlitSurface(ryt[i].valeur, NULL, fond, &ryt[i].coordv);
					}
				sprintf (nom, "Voix : %d", voixAct+1);
				bnVoix.cadre = TTF_RenderText_Shaded (police2, nom, bleu, bleu);
				if (P->paramVoix[voixAct] == 0)
					sprintf(nom, "Fondamentale forcee : non");
				else
					sprintf(nom, "Fondamentale forcee : oui");
				bbassForced.cadre = TTF_RenderText_Shaded(police3, nom, bleu, bleu);
				SDL_BlitSurface (bbassForced.cadre, NULL, fond, &bbassForced.coord);
				SDL_BlitSurface(bnVoix.cadre, NULL, fond, &bnVoix.coord);
				voixAct--;

				for (i = 0; i < 6; i++)
					{
					sprintf (nom, "%d", P->rythfreq[voixAct][i]);
					ryt[i].valeur = TTF_RenderText_Shaded (police2, nom, orange, bleu);

					SDL_BlitSurface(ryt[i].valeur, NULL, fond, &ryt[i].coordv);
					}
				sprintf (nom, "Voix : %d", voixAct+1);
				bnVoix.cadre = TTF_RenderText_Shaded (police2, nom, orange, bleu);
				if (P->paramVoix[voixAct] == 0)
					sprintf(nom, "Fondamentale forcee : non");
				else
					sprintf(nom, "Fondamentale forcee : oui");
				bbassForced.cadre = TTF_RenderText_Shaded(police3, nom, orange, bleu);
				SDL_BlitSurface (bbassForced.cadre, NULL, fond, &bbassForced.coord);
				SDL_BlitSurface(bnVoix.cadre, NULL, fond, &bnVoix.coord);
				SDL_UpdateWindowSurface (fen);
				}//du inbout bflechem

			else if (inbout (event, bbassForced))
				{
				if (P->paramVoix[voixAct] == 0)
					sprintf(nom, "Fondamentale forcee : non");
				else
					sprintf(nom, "Fondamentale forcee : oui");
				bbassForced.cadre = TTF_RenderText_Shaded(police3, nom, bleu, bleu);
				SDL_BlitSurface (bbassForced.cadre, NULL, fond, &bbassForced.coord);
				if (P->paramVoix[voixAct] == 0)
					P->paramVoix[voixAct] = 1;
				else
					P->paramVoix[voixAct] = 0;

				if (P->paramVoix[voixAct] == 0)
					sprintf(nom, "Fondamentale forcee : non");
				else
					sprintf(nom, "Fondamentale forcee : oui");
				bbassForced.cadre = TTF_RenderText_Shaded(police3, nom, orange, bleu);
				SDL_BlitSurface (bbassForced.cadre, NULL, fond, &bbassForced.coord);
				SDL_UpdateWindowSurface (fen);
				}

			else if (inbout (event, bquitter))
				{
				SDL_FillRect(fond,NULL,SDL_MapRGB(fond->format, 10, 10, 200));
				SDL_BlitSurface (bquitter.cadre, NULL, fond, &bquitter.coord);
				SDL_BlitSurface (brythmes.cadre, NULL, fond, &brythmes.coord);
				SDL_BlitSurface (btonmode.cadre, NULL, fond, &btonmode.coord);
				SDL_BlitSurface (bnotes.cadre, NULL, fond, &bnotes.coord);
				SDL_BlitSurface (bcad.cadre, NULL, fond, &bcad.coord);
				SDL_BlitSurface (bvoix.cadre, NULL, fond, &bvoix.coord);
				SDL_UpdateWindowSurface (fen);
				continuer2 = 0;// Reblit' du menu paramètres
				}
			break;
		}
	}
}

//<=<=<=<=<=<=<=<=<=<=<=<=<=<=<=<=<=<=<<= Pour lisibilité
				else if (inbout (event, bnotes))// cf ligne 374
//<=<=<=<=<=<=<=<=<=<=<=<=<=<=<=<=<=<=<<= Pour lisibilité
{
SDL_FillRect(fond,NULL,SDL_MapRGB(fond->format, 10, 10, 200));
SDL_BlitSurface (bquitter.cadre, NULL, fond, &bquitter.coord);

for (i = 1; i <= 7; i++)
	{
	sprintf (nom, "Frequence du deg %d  : ", i);
	not[i].titre = TTF_RenderText_Shaded (police2, nom, orange, bleu);
	}

for (i = 1; i < 8; i++)
	{
	sprintf (nom, "%d", P->notefreq[i]);
	not[i].valeur = TTF_RenderText_Shaded (police2, nom, orange, bleu);

	not[i].plus = TTF_RenderText_Shaded (police,"+", bleu, orange);
	not[i].moins = TTF_RenderText_Shaded (police,"-", bleu, orange);
	not[i].coord.x = 50;
	not[i].coord.y = 5 +55*i;
	flipboutp(&not[i]);

	SDL_BlitSurface(not[i].titre, NULL, fond, &not[i].coord);
	SDL_BlitSurface(not[i].plus, NULL, fond, &not[i].coordp);
	SDL_BlitSurface(not[i].moins, NULL, fond, &not[i].coordm);
	SDL_BlitSurface(not[i].valeur, NULL, fond, &not[i].coordv);
	}
SDL_UpdateWindowSurface (fen);

continuer2 = 1;
while (continuer2)
	{
	SDL_WaitEvent (&event);
	switch (event.type)
		{
		case SDL_QUIT:
			exit (0);
		case SDL_MOUSEBUTTONUP:
			for (i = 1; i < 8; i++)
				if (inbout2 (event,not[i].coordp, not[i].plus)  && P->notefreq[i] < 100) 					{
					sprintf (nom, "%d", P->notefreq[i]);
					not[i].valeur = TTF_RenderText_Shaded (police2,nom, bleu, bleu);
					SDL_BlitSurface (not[i].valeur, NULL, fond, &not[i].coordv);
					P->notefreq[i]++;
					sprintf (nom, "%d", P->notefreq[i]);
					not[i].valeur = TTF_RenderText_Shaded (police2,nom, orange, bleu);
					SDL_BlitSurface (not[i].valeur, NULL, fond, &not[i].coordv);
					SDL_UpdateWindowSurface (fen);
					}
				else if (inbout2 (event,not[i].coordm, not[i].moins) && P->notefreq[i] > 0
	&& P->notefreq[1] + P->notefreq[2] + P->notefreq[3] + P->notefreq[4] + P->notefreq[5] + P->notefreq[6]+ P->notefreq[7] >0)
					{
					sprintf (nom, "%d", P->notefreq[i]);
					not[i].valeur = TTF_RenderText_Shaded (police2,nom, bleu, bleu);
					SDL_BlitSurface (not[i].valeur, NULL, fond, &not[i].coordv);
					P->notefreq[i]--;
					sprintf (nom, "%d", P->notefreq[i]);
					not[i].valeur = TTF_RenderText_Shaded (police2,nom, orange, bleu);
					SDL_BlitSurface (not[i].valeur, NULL, fond, &not[i].coordv);
					SDL_UpdateWindowSurface (fen);
					}
				else if (inbout (event, bquitter))
					{
					SDL_FillRect(fond,NULL,SDL_MapRGB(fond->format, 10, 10, 200));
					SDL_BlitSurface (bquitter.cadre, NULL, fond, &bquitter.coord);
					SDL_BlitSurface (brythmes.cadre, NULL, fond, &brythmes.coord);
					SDL_BlitSurface (btonmode.cadre, NULL, fond, &btonmode.coord);
					SDL_BlitSurface (bnotes.cadre, NULL, fond, &bnotes.coord);
					SDL_BlitSurface (bcad.cadre, NULL, fond, &bcad.coord);
					SDL_BlitSurface (bvoix.cadre, NULL, fond, &bvoix.coord);
					SDL_UpdateWindowSurface (fen);
					continuer2 = 0;// Reblit' du menu paramètres
					}

			break;
		}
	}
}
//<=<=<=<=<=<=<=<=<=<=<=<=<=<=<=<=<=<=<<= Pour lisibilité

				else if (inbout (event, btonmode))
//<=<=<=<=<=<=<=<=<=<=<=<=<=<=<=<=<=<=<<= Pour lisibilité
{
continuer2 = 1;
SDL_FillRect(fond,NULL,SDL_MapRGB(fond->format, 10, 10, 200));
SDL_BlitSurface (bquitter.cadre, NULL, fond, &bquitter.coord);

tempo.titre = TTF_RenderText_Shaded (police, "Tempo : ", orange, bleu);//Premier blit du tempo
tempo.plus = TTF_RenderText_Shaded (police, "+", bleu, orange);
tempo.moins = TTF_RenderText_Shaded (police, "-", bleu, orange);
sprintf(nom, "%d", P->tempoBPM);
tempo.valeur = TTF_RenderText_Shaded (police,nom , orange, bleu);
tempo.coord.x = 70;
tempo.coord.y = 100;
flipboutp(&tempo);
bTempop.cadre = TTF_RenderText_Shaded (police2, "+15", bleu, orange);
bTempom.cadre = TTF_RenderText_Shaded (police2, "-15", bleu, orange);
bTempop.coord.x = tempo.coordp.x + tempo.plus->w + 10;
bTempop.coord.y = tempo.coord.y;
bTempom.coord.x = tempo.coordm.x - bTempom.cadre->w -10;
bTempom.coord.y = tempo.coord.y;

tons.titre = TTF_RenderText_Shaded (police, "Tonalite : ", orange, bleu);//Premier blit de la tonalité
tons.plus = TTF_RenderText_Shaded (police, "+", bleu, orange);
tons.moins = TTF_RenderText_Shaded (police, "-", bleu, orange);
sprintf(nom, "%c", nomtonalites[4*P->tonalite]);
sprintf(nom, "%s%c", nom, nomtonalites[4*P->tonalite + 1]);
sprintf(nom, "%s%c", nom, nomtonalites[4*P->tonalite + 2]);
sprintf(nom, "%s%c", nom, nomtonalites[4*P->tonalite + 3]);
tons.valeur = TTF_RenderText_Shaded (police,nom , orange, bleu);
tons.coord.x = 70;
tons.coord.y = 170;
flipboutp(&tons);
//tons.coordp.y += 30;

mode.titre = TTF_RenderText_Shaded (police, "Mode : ", orange, bleu);//premier blit de mode
mode.plus = TTF_RenderText_Shaded (police, "+", bleu, orange);
mode.moins = TTF_RenderText_Shaded (police, "-", bleu, orange);
nom[0] = '\0';
for (i = 0; i < 11; i++)
	sprintf(nom, "%s%c", nom ,nommodes[11*P->mode+i]);
mode.valeur = TTF_RenderText_Shaded (police,nom , orange, bleu);
mode.coord.x = 70;
mode.coord.y = 240;
flipboutp(&mode);
//mode.coordp.y += 30;

SDL_BlitSurface (mode.titre, NULL, fond, &mode.coord);
SDL_BlitSurface (mode.plus, NULL, fond, &mode.coordp);
SDL_BlitSurface (mode.moins, NULL, fond, &mode.coordm);
SDL_BlitSurface (mode.valeur, NULL, fond, &mode.coordv);
SDL_BlitSurface (tons.titre, NULL, fond, &tons.coord);
SDL_BlitSurface (tons.plus, NULL, fond, &tons.coordp);
SDL_BlitSurface (tons.moins, NULL, fond, &tons.coordm);
SDL_BlitSurface (tons.valeur, NULL, fond, &tons.coordv);
SDL_BlitSurface (tempo.titre, NULL, fond, &tempo.coord);
SDL_BlitSurface (tempo.plus, NULL, fond, &tempo.coordp);
SDL_BlitSurface (tempo.moins, NULL, fond, &tempo.coordm);
SDL_BlitSurface (tempo.valeur, NULL, fond, &tempo.coordv);
SDL_BlitSurface (bTempop.cadre, NULL, fond, &bTempop.coord);
SDL_BlitSurface (bTempom.cadre, NULL, fond, &bTempom.coord);


SDL_UpdateWindowSurface (fen);

while (continuer2)
	{
	SDL_WaitEvent (&event);
	switch (event.type)
		{
		case SDL_QUIT:
			exit (0);
			break;
		case SDL_MOUSEBUTTONUP:
			if (inbout2 (event, tempo.coordp, tempo.plus) && P->tempoBPM < 300)
				{//Bouton Tempo+ : on ajoute 1 et refait l'affichage
				sprintf (nom, "%d", P->tempoBPM);
				tempo.valeur = TTF_RenderText_Shaded (police,nom, bleu, bleu);
				SDL_BlitSurface (tempo.valeur, NULL, fond, &tempo.coordv);
				P->tempoBPM++;
				sprintf (nom, "%d", P->tempoBPM);
				tempo.valeur = TTF_RenderText_Shaded (police,nom, orange, bleu);
				SDL_BlitSurface (tempo.valeur, NULL, fond, &tempo.coordv);
				SDL_UpdateWindowSurface (fen);
				}
			else if (inbout2 (event, tempo.coordm, tempo.moins) && P->tempoBPM > 20)
				{//bouton Tempo- : on retire 1 et refait l'affichage
				sprintf (nom, "%d", P->tempoBPM);
				tempo.valeur = TTF_RenderText_Shaded (police,nom, bleu, bleu);
				SDL_BlitSurface (tempo.valeur, NULL, fond, &tempo.coordv);
				P->tempoBPM--;
				sprintf (nom, "%d", P->tempoBPM);
				tempo.valeur = TTF_RenderText_Shaded (police,nom, orange, bleu);
				SDL_BlitSurface (tempo.valeur, NULL, fond, &tempo.coordv);
				SDL_UpdateWindowSurface (fen);
				}

			else if (inbout(event, bTempom) && P->tempoBPM > 20)
				{//bouton Tempo- : on retire 1 et refait l'affichage
				sprintf (nom, "%d", P->tempoBPM);
				tempo.valeur = TTF_RenderText_Shaded (police,nom, bleu, bleu);
				SDL_BlitSurface (tempo.valeur, NULL, fond, &tempo.coordv);
				P->tempoBPM -= 15;
				sprintf (nom, "%d", P->tempoBPM);
				tempo.valeur = TTF_RenderText_Shaded (police,nom, orange, bleu);
				SDL_BlitSurface (tempo.valeur, NULL, fond, &tempo.coordv);
				SDL_UpdateWindowSurface (fen);
				}
			else if (inbout (event, bTempop) && P->tempoBPM < 300)
				{//Bouton Tempo+ : on ajoute 1 et refait l'affichage
				sprintf (nom, "%d", P->tempoBPM);
				tempo.valeur = TTF_RenderText_Shaded (police,nom, bleu, bleu);
				SDL_BlitSurface (tempo.valeur, NULL, fond, &tempo.coordv);
				P->tempoBPM += 15;
				sprintf (nom, "%d", P->tempoBPM);
				tempo.valeur = TTF_RenderText_Shaded (police,nom, orange, bleu);
				SDL_BlitSurface (tempo.valeur, NULL, fond, &tempo.coordv);
				SDL_UpdateWindowSurface (fen);
				}


			else if (inbout2 (event, mode.coordp, mode.plus))
				{//Bouton Mode + : on passe au mode "suivant"
				nom[0] = '\0';
				for (i = 0; i < 11; i++)
					sprintf (nom, "%s%c", nom, nommodes[11*P->mode+i]);
				mode.valeur = TTF_RenderText_Shaded (police,nom, bleu, bleu);
				SDL_BlitSurface (mode.valeur, NULL, fond, &mode.coordv);
				if (P->mode == 2)
					P->mode = 0;
				else
					P->mode++;
				nom[0] = '\0';
				for (i = 0; i < 11; i++)
					sprintf (nom, "%s%c", nom, nommodes[11*P->mode+i]);
				mode.valeur = TTF_RenderText_Shaded (police,nom, orange, bleu);
				SDL_BlitSurface (mode.valeur, NULL, fond, &mode.coordv);
				SDL_UpdateWindowSurface (fen);
				}

			else if (inbout2 (event, mode.coordm, mode.moins))
				{//Bouton mode- : on passe au mode "précédent"
				nom[0] = '\0';
				for (i = 0; i < 11; i++)
					sprintf (nom, "%s%c", nom, nommodes[11*P->mode+i]);
				mode.valeur = TTF_RenderText_Shaded (police,nom, bleu, bleu);
				SDL_BlitSurface (mode.valeur, NULL, fond, &mode.coordv);
				if (P->mode == 0)
					P->mode = 2;
				else
					P->mode--;
				nom[0] = '\0';
				for (i = 0; i < 11; i++)
					sprintf (nom, "%s%c", nom, nommodes[11*P->mode+i]);
				mode.valeur = TTF_RenderText_Shaded (police,nom, orange, bleu);
				SDL_BlitSurface (mode.valeur, NULL, fond, &mode.coordv);
				SDL_UpdateWindowSurface (fen);
				}

			else if (inbout2 (event, tons.coordp, tons.plus))
				{//Bouton tonalite+ : on passe à la tonalité un demi-ton au dessus
				nom[0] = '\0';
				for (i = 0; i < 4; i++)
					sprintf (nom, "%s%c", nom, nomtonalites[4*P->tonalite+i]);
				tons.valeur = TTF_RenderText_Shaded (police,nom, bleu, bleu);
				SDL_BlitSurface (tons.valeur, NULL, fond, &tons.coordv);
				if (P->tonalite >= 11)
					P->tonalite = 0;
				else
					P->tonalite++;
				nom[0] = '\0';
				for (i = 0; i < 4; i++)
					sprintf (nom, "%s%c", nom, nomtonalites[4*P->tonalite+i]);
				tons.valeur = TTF_RenderText_Shaded (police,nom, orange, bleu);
				SDL_BlitSurface (tons.valeur, NULL, fond, &tons.coordv);
				SDL_UpdateWindowSurface (fen);
				}
			else if (inbout2 (event, tons.coordm, tons.moins))
				{//Bouton tonalite- : on passe à la tonalité un demi-ton en dessous
				nom[0] = '\0';
				for (i = 0; i < 4; i++)
					sprintf (nom, "%s%c", nom, nomtonalites[4*P->tonalite+i]);
				tons.valeur = TTF_RenderText_Shaded (police,nom, bleu, bleu);
				SDL_BlitSurface (tons.valeur, NULL, fond, &tons.coordv);
				if (P->tonalite == 0)
					P->tonalite = 11;
				else
					P->tonalite--;
				nom[0] = '\0';
				for (i = 0; i < 4; i++)
					sprintf (nom, "%s%c", nom, nomtonalites[4*P->tonalite+i]);
				tons.valeur = TTF_RenderText_Shaded (police,nom, orange, bleu);
				SDL_BlitSurface (tons.valeur, NULL, fond, &tons.coordv);
				SDL_UpdateWindowSurface (fen);
				}
			else if (inbout (event, bquitter))
				{
				SDL_FillRect(fond,NULL,SDL_MapRGB(fond->format, 10, 10, 200));
				SDL_BlitSurface (bquitter.cadre, NULL, fond, &bquitter.coord);
				SDL_BlitSurface (brythmes.cadre, NULL, fond, &brythmes.coord);
				SDL_BlitSurface (bnotes.cadre, NULL, fond, &bnotes.coord);
				SDL_BlitSurface (btonmode.cadre, NULL, fond, &btonmode.coord);
				SDL_BlitSurface (bcad.cadre, NULL, fond, &bcad.coord);
				SDL_BlitSurface (bvoix.cadre, NULL, fond, &bvoix.coord);
				SDL_UpdateWindowSurface (fen);
				continuer2 = 0;
				}
			break;
		}
	}
if (gammeAvant != P->tonalite || modeAvant != P->mode)
	{
/**/	for (i = 0; i < P->nbreInstru; i++)
		for (l = 0; l < 8; l++)
			for (m = 0; m < 7; m++)
				{
				if (P->modele[i][l].num[m] != NULL && P->modele[i][l].num[m] != P->err)
					{
					Mix_FreeChunk (P->modele[i][l].num[m]);
					P->modele[i][l].num[m] = NULL;
					}
				else
					P->modele[i][l].num[m] = NULL;
				}
			// Libération de tout les sons chargés, une seule fois par instrument (Car s'il y a deux voix d'un même
			//    instrument, ces deux voix contiennent les mêmes pointeurs. Ainsi, les sons en eux-même ne sont
			//     présent qu'une fois dans la mémoire. Mais on ne peut pas Mix_FreeChunk'er twice un pointeur !
	loadgamme(P);//			--> Ok, puis j'ai amélioré donc plus besoin
	}

}
//<=<=<=<=<=<=<=<=<=<=<=<=<=<=<=<=<=<=<<= Pour lisibilité
				break;
			}//swtich

		}//while (continuer)
	//et ici la suite: (à faire avant de retourner ds le menu)

	for (i = 0; i < P->nbreInstru; i++)
		for (j = 0; j < 8; j++)
			for (k = 0; k < 7; k++)
				{
				if (P->modele[i][j].num[k] != NULL)
					Mix_VolumeChunk(P->modele[i][j].num[k], MIX_MAX_VOLUME * P->volInstru[i] / 100);
				}//Mise à jour des volumes


	k = 0;
	printf ("\nBankry : ");

	for (voixAct = 0; voixAct < P->nbreVoix; voixAct++)
		{
		for (i = 0; i < 7; i++)
			{
			for (j = 0; j < P->rythfreq[voixAct][i] && j < 500/7/*7degrés, 500places*/; j++)	{
				P->bankry[voixAct][k] = i;//	Un nombre => un rythme : 0 = ronde, 1 = blanche, 2 = noire...
				printf("%d,",P->bankry[voixAct][k]);
				k++;				}
			}// Actualisation du tableau de rythmes
		P->bankry[voixAct][k] = -1;
		}
	printf ("\nBankn : ");
	k = 0;
	for (i = 1; i < 8; i++)
		{
		for (j = 0; j < P->notefreq[i] && j < 500/6/*6rythmes, 500 places*/; j++) 	{
			P->bankn[k] = i;
			printf ("%d,",P->bankn[k]);
			k++;				}
		}// 	Ici, P->bankn[k] contient les numeros des degré le nombre de fois indiqué dans le ccfreq correspondant
	P->bankn[k] = -1;
	k = 0;
	for (i = 0; i < 5; i++)
		{
		for (j = 0; j < P->cadfreq[i]; j++)	{
			P->bankcad[k] = i;
			k++;				}
		}//Mise à jour de bankcad
	P->mspn = 60000/P->tempoBPM;

	return (1);
}//fonction. Ouf !


//bide
