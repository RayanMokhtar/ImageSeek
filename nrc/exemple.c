#include <stdio.h>
#include <stdlib.h>
#include "def.h"
#include "nrio.h"
#include "nrarith.h"
#include "nralloc.h"


byte** filtre_moyenneur(byte** image, long nrl, long nrh, long ncl, long nch) {
    byte** imagefiltre = bmatrix(nrl, nrh, ncl, nch);
    
	int sommeFiltre = 9 ;
	int filtre[3][3] = {{1, 1, 1}, {1, 1, 1}, {1, 1, 1}};
    for (long i = nrl + 1; i <= nrh - 1; i++) {
        for (long j = ncl + 1; j <= nch - 1; j++) {
            int sum = 0;
            for (int di = -1; di <= 1; di++) {
                for (int dj = -1; dj <= 1; dj++) {
                    sum += image[i + di][j + dj] * filtre[di + 1][dj + 1];
                }
            }
            imagefiltre[i][j] = sum / sommeFiltre;
        }
    }
    
    return imagefiltre;
}

byte **masque_sobel(byte** image, long nrl, long nrh, long ncl, long nch){
	byte** imagefiltre = bmatrix(nrl, nrh, ncl, nch);
	int sommeFiltreSobel = 4;
	int matrice_gradient[3][3] = {{-1, -2, -1}, {0, 0, 0}, {1, 2, 1}};
	for (long i = nrl + 1; i <= nrh - 1; i++) {
		for (long j = ncl + 1; j <= nch - 1; j++) {
			int sum = 0;
			for (int di = -1; di <= 1; di++) {
				for (int dj = -1; dj <= 1 ; dj++) {
					sum += image[i + di][j + dj] * matrice_gradient[di + 1][dj + 1];
				}
			}
			
			imagefiltre[i][j] = abs(sum / sommeFiltreSobel);
		}
	}

	return imagefiltre;
}


byte **norme_gradient(byte** I , long nrl, long nrh, long ncl, long nch ) {
	byte** mat_grad = bmatrix(nrl, nrh, ncl, nch);

	for(int i = nrl ; i <= nrh ; i++){
		for(int j = ncl ; j <= nch ; j++){
			mat_grad[i][j] = sqrt(I[i][j]*I[i][j] + I[i][j]*I[i][j]);
		}
	}

	return mat_grad;
}

byte **binarisation(byte**I , long nrl , long nrh , long ncl , long nch){

	byte** mat_binarisation = bmatrix(nrl, nrh, ncl, nch);

	for(int i = nrl ; i <= nrh ; i++){
		for(int j = ncl ; j <= nch ; j++){
			if(I[i][j] > 128){
				mat_binarisation[i][j] = 255;
			}else{
				mat_binarisation[i][j] = 0;
			}
		}
	}
	return mat_binarisation;


}

byte **segmentation(byte** I , long nrl , long nrh , long ncl , long nch){
	byte** mat_segmentation = bmatrix(nrl, nrh, ncl, nch);
	int seuil = 128;
	for(int i = nrl ; i <= nrh ; i++){
		for(int j = ncl ; j <= nch ; j++){
			if(I[i][j] > seuil){
				mat_segmentation[i][j] = I[i][j];
			}else{
				mat_segmentation[i][j] = 0;
			}
		}
	}

	return mat_segmentation;
}


int trouver_max_matrice(byte** I , long nrl , long nrh , long ncl , long nch){
	int max = 0;
	for(int i = nrl ; i <= nrh ; i++){
		for(int j = ncl ; j <= nch ; j++){
			if(I[i][j] > max){
				max = I[i][j];
			}
		}
	}
	return max;
}

int normalisation(byte ** I){
	int max = trouver_max_matrice(I,0,255,0,255);
	for(int i = 0 ; i <= 255 ; i++){
		for(int j = 0 ; j <= 255 ; j++){
			I[i][j] = (I[i][j] * 255) / max;
		}
	}
}

byte **composantes_connexes(){
	int composante_connexe = 0;

	printf("composantes connexes %d\n",composante_connexe);
}


int main(void) {
    long nrh, nrl, nch, ncl;
    byte** I;
    byte** R;
	byte ** sobel ;
	byte ** image_superposee;


    I = LoadPGM_bmatrix("cubes.pgm", &nrl, &nrh, &ncl, &nch); // iamge en byte
    R = filtre_moyenneur(I, nrl, nrh, ncl, nch); // application du filtre

	image_superposee = norme_gradient(I, nrl, nrh, ncl, nch); // application du masque de sobel 
 
	
	// SavePGM_bmatrix(I, nrl, nrh, ncl, nch, "cubes.pgm");
    // SavePGM_bmatrix(R, nrl, nrh, ncl, nch, "cubes_moyenneur.pgm");x
	// SavePGM_bmatrix(sobel,nrl,nrh,ncl,nch,"cubes_sobel.pgm");
	SavePGM_bmatrix(I, nrl, nrh, ncl, nch, "cubes_normegradient.pgm");
	
    free_bmatrix(I, nrl, nrh, ncl, nch);
    free_bmatrix(R, nrl, nrh, ncl, nch);

    return 0;
}