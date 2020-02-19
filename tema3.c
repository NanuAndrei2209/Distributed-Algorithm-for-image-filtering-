#include<mpi.h>
#include<string.h>
#include<stdlib.h>
#include<stdio.h>
#define min(a,b) (((a)<(b))?(a):(b))
#define max(a,b) (((a)>(b))?(a):(b))

int master = 0;


int main(int argc, char * argv[]) {
	int rank;
	int nProcesses;

	MPI_Init(&argc, &argv);
	MPI_Status status;
	MPI_Request request;

	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &nProcesses);

	float smoothMatrix[3][3];
	for(int i = 0; i < 3; ++i) {
		for(int j = 0; j < 3; ++j) {
			smoothMatrix[i][j] = 1.0f / 9.0f;
		}
	}
	
	float blurMatrix[3][3];
	blurMatrix[0][0] = 1.0f / 16.0f;
	blurMatrix[0][1] = 2.0f / 16.0f;
	blurMatrix[0][2] = 1.0f / 16.0f;
	blurMatrix[1][0] = 2.0f / 16.0f;
	blurMatrix[1][1] = 4.0f / 16.0f;
	blurMatrix[1][2] = 2.0f / 16.0f;
	blurMatrix[2][0] = 1.0f / 16.0f;
	blurMatrix[2][1] = 2.0f / 16.0f;
	blurMatrix[2][2] = 1.0f / 16.0f;
	
	float sharpenMatrix[3][3];
	sharpenMatrix[0][0] = 0.0f / 3.0f;
	sharpenMatrix[0][1] = -2.0f / 3.0f;
	sharpenMatrix[0][2] = 0.0f / 3.0f;
	sharpenMatrix[1][0] = -2.0f / 3.0f;
	sharpenMatrix[1][1] = 11.0f / 3.0f;
	sharpenMatrix[1][2] = -2.0f / 3.0f;
	sharpenMatrix[2][0] = 0.0f / 3.0f;
	sharpenMatrix[2][1] = -2.0f / 3.0f;
	sharpenMatrix[2][2] = 0.0f / 3.0f;
	
	float meanMatrix[3][3];
	meanMatrix[0][0] = -1.0f;
	meanMatrix[0][1] = -1.0f;
	meanMatrix[0][2] = -1.0f;
	meanMatrix[1][0] = -1.0f;
	meanMatrix[1][1] = 9.0f;
	meanMatrix[1][2] = -1.0f;
	meanMatrix[2][0] = -1.0f;
	meanMatrix[2][1] = -1.0f;
	meanMatrix[2][2] = -1.0f;
	
	float embossMatrix[3][3];
	embossMatrix[0][0] = 0.0f;
	embossMatrix[0][1] = 1.0f;
	embossMatrix[0][2] = 0.0f;
	embossMatrix[1][0] = 0.0f;
	embossMatrix[1][1] = 0.0f;
	embossMatrix[1][2] = 0.0f;
	embossMatrix[2][0] = 0.0f;
	embossMatrix[2][1] = -1.0f;
	embossMatrix[2][2] = 0.0f;

	if (rank == master) {

		FILE *f;
		f = fopen(argv[1], "r");
		unsigned char** blackWhite;
		unsigned char** red;
		unsigned char** green;
		unsigned char** blue;
		int P, height, width, maxValue;
		char c;
		char s[100];


		fscanf(f, "%c", &c);
		fscanf(f, "%d", &P);
		printf("P%d\n", P);

		fscanf(f, "%s", s);
		fscanf(f, "%s", s);
		fscanf(f, "%s", s);
		fscanf(f, "%s", s);
		fscanf(f, "%s", s);
		fscanf(f, "%s", s);
		fscanf(f, "%s", s);
		fscanf(f, "%s", s);
		
		char *s1 = strdup((const char*)"# Created by GIMP version 2.10.14 PNM plug-in\n");
		//printf("%s\n", s1);
		

		fscanf(f, "%d", &width);
		fscanf(f, "%d", &height);
		fscanf(f, "%d", &maxValue);
		fscanf(f, "%c", &c);
		

		if (P == 5) {
			blackWhite = (unsigned char **) malloc ((height + 2) * sizeof(unsigned char*));

			for (int i = 0; i < height + 2; ++i) {
				blackWhite[i] = (unsigned char*) malloc ((width + 2) * sizeof(unsigned char));
			}

			for (int i = 0; i < height + 2; ++i) {
				for (int j = 0; j < width + 2; ++j) {
					if (i == 0 || j == 0 || i == height + 1 || j == width + 1) {
						blackWhite[i][j] = 0;
					} else {
						fscanf(f, "%c", &c);
						blackWhite[i][j] = c;
					}
				}
			}

			int low, high;
			unsigned char **newBW = (unsigned char **) malloc ((height + 2) * sizeof(unsigned char*));
			
			for (int i = 0; i < height + 2; ++i) {
				newBW[i] = (unsigned char*) malloc ((width + 2) * sizeof(unsigned char));
			}

			for (int i = 1; i < nProcesses; ++i) {
				
				low = max((height / nProcesses) * (i - 1), 1) - 1;
				high = (height / nProcesses) * i + 1;

				int nrLines = high - low + 1;

				MPI_Send(&P, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
				MPI_Send(&height, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
				MPI_Send(&width, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
				MPI_Send(&nrLines, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
			}

			int k = 3;
			while (k < argc) {

				if (nProcesses != 1) {

					// send
					for (int i = 1; i < nProcesses; ++i) {
						low = max((height / nProcesses) * (i - 1), 1) - 1;
						high = (height / nProcesses) * i + 1;
						for (int j = low; j <= high; ++j) {
							MPI_Send(blackWhite[j], width + 2, MPI_CHAR, i, 0, MPI_COMM_WORLD);
						}
					}
					
					// calcul
					for (int i = (height / nProcesses) * (nProcesses - 1) - 1; i < height + 1; ++i) {
						for (int j = 1; j < width + 1; ++j) {
							if (strcmp(argv[k], "smooth") == 0) {
								
								float x = smoothMatrix[2][2] * blackWhite[i - 1][j - 1]
										+ smoothMatrix[2][1] * blackWhite[i - 1][j]
										+ smoothMatrix[2][0] * blackWhite[i - 1][j + 1]
										+ smoothMatrix[1][2] * blackWhite[i][j - 1]
										+ smoothMatrix[1][1] * blackWhite[i][j]
										+ smoothMatrix[1][0] * blackWhite[i][j + 1]
										+ smoothMatrix[0][2] * blackWhite[i + 1][j - 1]
										+ smoothMatrix[0][1] * blackWhite[i + 1][j]
										+ smoothMatrix[0][0] * blackWhite[i + 1][j + 1];
								
								if (x > 255.0f) {
									x = 255.0f;
								} else if (x < 0.0f) {
									x = 0.0f;
								}
								newBW[i][j] = (unsigned char) x;
								
									
							} else if (strcmp(argv[k], "blur") == 0) {
								float x = blurMatrix[2][2] * blackWhite[i - 1][j - 1]
										+ blurMatrix[2][1] * blackWhite[i - 1][j]
										+ blurMatrix[2][0] * blackWhite[i - 1][j + 1]
										+ blurMatrix[1][2] * blackWhite[i][j - 1]
										+ blurMatrix[1][1] * blackWhite[i][j]
										+ blurMatrix[1][0] * blackWhite[i][j + 1]
										+ blurMatrix[0][2] * blackWhite[i + 1][j - 1]
										+ blurMatrix[0][1] * blackWhite[i + 1][j]
										+ blurMatrix[0][0] * blackWhite[i + 1][j + 1];
								
								if (x > 255.0f) {
									x = 255.0f;
								} else if (x < 0.0f) {
									x = 0.0f;
								}
								newBW[i][j] = (unsigned char) x;
								
							} else if (strcmp(argv[k], "mean") == 0) {
								float x = meanMatrix[2][2] * blackWhite[i - 1][j - 1]
										+ meanMatrix[2][1] * blackWhite[i - 1][j]
										+ meanMatrix[2][0] * blackWhite[i - 1][j + 1]
										+ meanMatrix[1][2] * blackWhite[i][j - 1]
										+ meanMatrix[1][1] * blackWhite[i][j]
										+ meanMatrix[1][0] * blackWhite[i][j + 1]
										+ meanMatrix[0][2] * blackWhite[i + 1][j - 1]
										+ meanMatrix[0][1] * blackWhite[i + 1][j]
										+ meanMatrix[0][0] * blackWhite[i + 1][j + 1];
								
								if (x > 255.0f) {
									x = 255.0f;
								} else if (x < 0.0f) {
									x = 0.0f;
								}
								newBW[i][j] = (unsigned char) x;
								
							} else if (strcmp(argv[k], "emboss") == 0) {
								float x = embossMatrix[2][2] * blackWhite[i - 1][j - 1]
										+ embossMatrix[2][1] * blackWhite[i - 1][j]
										+ embossMatrix[2][0] * blackWhite[i - 1][j + 1]
										+ embossMatrix[1][2] * blackWhite[i][j - 1]
										+ embossMatrix[1][1] * blackWhite[i][j]
										+ embossMatrix[1][0] * blackWhite[i][j + 1]
										+ embossMatrix[0][2] * blackWhite[i + 1][j - 1]
										+ embossMatrix[0][1] * blackWhite[i + 1][j]
										+ embossMatrix[0][0] * blackWhite[i + 1][j + 1];
								
								if (x > 255.0f) {
									x = 255.0f;
								} else if (x < 0.0f) {
									x = 0.0f;
								}
								newBW[i][j] = (unsigned char) x;

							} else if (strcmp(argv[k], "sharpen") == 0) {
								float x = sharpenMatrix[2][2] * blackWhite[i - 1][j - 1]
										+ sharpenMatrix[2][1] * blackWhite[i - 1][j]
										+ sharpenMatrix[2][0] * blackWhite[i - 1][j + 1]
										+ sharpenMatrix[1][2] * blackWhite[i][j - 1]
										+ sharpenMatrix[1][1] * blackWhite[i][j]
										+ sharpenMatrix[1][0] * blackWhite[i][j + 1]
										+ sharpenMatrix[0][2] * blackWhite[i + 1][j - 1]
										+ sharpenMatrix[0][1] * blackWhite[i + 1][j]
										+ sharpenMatrix[0][0] * blackWhite[i + 1][j + 1];
								
								if (x > 255.0f) {
									x = 255.0f;
								} else if (x < 0.0f) {
									x = 0.0f;
								}
								newBW[i][j] = (unsigned char) x;
							}
						}
					}

					for (int i = (height / nProcesses) * (nProcesses - 1); i < height + 1; ++i) {
						for (int j = 1; j < width + 1; ++j) {
							blackWhite[i][j] = newBW[i][j];
						}
					}

					// recv
					for (int i = 1; i < nProcesses; ++i) {

						low = max((height / nProcesses) * (i - 1), 1) - 1;
						high = (height / nProcesses) * i + 1;

						MPI_Recv(newBW[0], width + 2, MPI_CHAR, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
						
						for (int j = low + 1; j < high; ++j) {
							MPI_Recv(blackWhite[j], width + 2, MPI_CHAR, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
						}
						
						MPI_Recv(newBW[0], width + 2, MPI_CHAR, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
					}

				} else {
					for (int i = 1; i < height + 1; ++i) {
						for (int j = 1; j < width + 1; ++j) {
							if (strcmp(argv[k], "smooth") == 0) {
								
								float x = smoothMatrix[2][2] * blackWhite[i - 1][j - 1]
										+ smoothMatrix[2][1] * blackWhite[i - 1][j]
										+ smoothMatrix[2][0] * blackWhite[i - 1][j + 1]
										+ smoothMatrix[1][2] * blackWhite[i][j - 1]
										+ smoothMatrix[1][1] * blackWhite[i][j]
										+ smoothMatrix[1][0] * blackWhite[i][j + 1]
										+ smoothMatrix[0][2] * blackWhite[i + 1][j - 1]
										+ smoothMatrix[0][1] * blackWhite[i + 1][j]
										+ smoothMatrix[0][0] * blackWhite[i + 1][j + 1];
								
								if (x > 255.0f) {
									x = 255.0f;
								} else if (x < 0.0f) {
									x = 0.0f;
								}
								newBW[i][j] = (unsigned char) x;
								
									
							} else if (strcmp(argv[k], "blur") == 0) {
								float x = blurMatrix[2][2] * blackWhite[i - 1][j - 1]
										+ blurMatrix[2][1] * blackWhite[i - 1][j]
										+ blurMatrix[2][0] * blackWhite[i - 1][j + 1]
										+ blurMatrix[1][2] * blackWhite[i][j - 1]
										+ blurMatrix[1][1] * blackWhite[i][j]
										+ blurMatrix[1][0] * blackWhite[i][j + 1]
										+ blurMatrix[0][2] * blackWhite[i + 1][j - 1]
										+ blurMatrix[0][1] * blackWhite[i + 1][j]
										+ blurMatrix[0][0] * blackWhite[i + 1][j + 1];
								
								if (x > 255.0f) {
									x = 255.0f;
								} else if (x < 0.0f) {
									x = 0.0f;
								}
								newBW[i][j] = (unsigned char) x;
								
							} else if (strcmp(argv[k], "mean") == 0) {
								float x = meanMatrix[2][2] * blackWhite[i - 1][j - 1]
										+ meanMatrix[2][1] * blackWhite[i - 1][j]
										+ meanMatrix[2][0] * blackWhite[i - 1][j + 1]
										+ meanMatrix[1][2] * blackWhite[i][j - 1]
										+ meanMatrix[1][1] * blackWhite[i][j]
										+ meanMatrix[1][0] * blackWhite[i][j + 1]
										+ meanMatrix[0][2] * blackWhite[i + 1][j - 1]
										+ meanMatrix[0][1] * blackWhite[i + 1][j]
										+ meanMatrix[0][0] * blackWhite[i + 1][j + 1];
								
								if (x > 255.0f) {
									x = 255.0f;
								} else if (x < 0.0f) {
									x = 0.0f;
								}
								newBW[i][j] = (unsigned char) x;
								
							} else if (strcmp(argv[k], "emboss") == 0) {
								float x = embossMatrix[2][2] * blackWhite[i - 1][j - 1]
										+ embossMatrix[2][1] * blackWhite[i - 1][j]
										+ embossMatrix[2][0] * blackWhite[i - 1][j + 1]
										+ embossMatrix[1][2] * blackWhite[i][j - 1]
										+ embossMatrix[1][1] * blackWhite[i][j]
										+ embossMatrix[1][0] * blackWhite[i][j + 1]
										+ embossMatrix[0][2] * blackWhite[i + 1][j - 1]
										+ embossMatrix[0][1] * blackWhite[i + 1][j]
										+ embossMatrix[0][0] * blackWhite[i + 1][j + 1];
								
								if (x > 255.0f) {
									x = 255.0f;
								} else if (x < 0.0f) {
									x = 0.0f;
								}
								newBW[i][j] = (unsigned char) x;

							} else if (strcmp(argv[k], "sharpen") == 0) {
								float x = sharpenMatrix[2][2] * blackWhite[i - 1][j - 1]
										+ sharpenMatrix[2][1] * blackWhite[i - 1][j]
										+ sharpenMatrix[2][0] * blackWhite[i - 1][j + 1]
										+ sharpenMatrix[1][2] * blackWhite[i][j - 1]
										+ sharpenMatrix[1][1] * blackWhite[i][j]
										+ sharpenMatrix[1][0] * blackWhite[i][j + 1]
										+ sharpenMatrix[0][2] * blackWhite[i + 1][j - 1]
										+ sharpenMatrix[0][1] * blackWhite[i + 1][j]
										+ sharpenMatrix[0][0] * blackWhite[i + 1][j + 1];
								
								if (x > 255.0f) {
									x = 255.0f;
								} else if (x < 0.0f) {
									x = 0.0f;
								}
								newBW[i][j] = (unsigned char) x;
							}
						}
					}
					for (int i = 1; i < height + 1; ++i) {
						for (int j = 1; j < width + 1; ++j) {
							blackWhite[i][j] = newBW[i][j];
						}
					}

				}
				k++;
			}

			// scriere fisier
			FILE *out;
			out = fopen(argv[2], "w");
			char ch = 'P';
			fprintf(out, "%c", ch);
			fprintf(out, "%d\n", P);
			fprintf(out, "%s", s1);
			free(s1);
			fprintf(out, "%d %d\n", width, height);
			fprintf(out, "%d\n", maxValue);

			for (int i = 1; i < height + 1; ++i) {
				for (int j = 1; j < width + 1; ++j) {
					fprintf(out, "%c", blackWhite[i][j]);
				}
			}
			fclose(out);
		} else {

			// citire
			red = (unsigned char **) malloc ((height + 2) * sizeof(unsigned char*));
			green = (unsigned char **) malloc ((height + 2) * sizeof(unsigned char*));
			blue = (unsigned char **) malloc ((height + 2) * sizeof(unsigned char*));

			for (int i = 0; i < height + 2; ++i) {
				red[i] = (unsigned char*) malloc ((width + 2) * sizeof(unsigned char));
				green[i] = (unsigned char*) malloc ((width + 2) * sizeof(unsigned char));
				blue[i] = (unsigned char*) malloc ((width + 2) * sizeof(unsigned char));
			}

			for (int i = 0; i < height + 2; ++i) {
				for (int j = 0; j < width + 2; ++j) {
					if (i == 0 || j == 0 || i == height + 1 || j == width + 1) {
						red[i][j] = 0;
						green[i][j] = 0;
						blue[i][j] = 0;
					} else {
						fscanf(f, "%c", &c);
						red[i][j] = c;
						fscanf(f, "%c", &c);
						green[i][j] = c;
						fscanf(f, "%c", &c);
						blue[i][j] = c;
					}
				}
			}

			// trimitere linii
			int low, high;
			
			unsigned char **newRed = (unsigned char **) malloc ((height + 2) * sizeof(unsigned char*));
			unsigned char **newGreen = (unsigned char **) malloc ((height + 2) * sizeof(unsigned char*));
			unsigned char **newBlue = (unsigned char **) malloc ((height + 2) * sizeof(unsigned char*));

			for (int i = 0; i < height + 2; ++i) {
				newRed[i] = (unsigned char*) malloc ((width + 2) * sizeof(unsigned char));
				newGreen[i] = (unsigned char*) malloc ((width + 2) * sizeof(unsigned char));
				newBlue[i] = (unsigned char*) malloc ((width + 2) * sizeof(unsigned char));
			}
			for (int i = 1; i < nProcesses; ++i) {
				
				low = max((height / nProcesses) * (i - 1), 1) - 1;
				high = (height / nProcesses) * i + 1;

				int nrLines = high - low + 1;

				MPI_Send(&P, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
				MPI_Send(&height, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
				MPI_Send(&width, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
				MPI_Send(&nrLines, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
			}

			int k = 3;

			while (k < argc) {
				
				if (nProcesses != 1) {
					//send
					for (int i = 1; i < nProcesses; ++i) {

						low = max((height / nProcesses) * (i - 1), 1) - 1;
						high = (height / nProcesses) * i + 1;

						for (int j = low; j <= high; ++j) {
							MPI_Send(red[j], width + 2, MPI_CHAR, i, 0, MPI_COMM_WORLD);
							MPI_Send(green[j], width + 2, MPI_CHAR, i, 0, MPI_COMM_WORLD);
							MPI_Send(blue[j], width + 2, MPI_CHAR, i, 0, MPI_COMM_WORLD);
						}
					}

					// calcul
					for (int i = (height / nProcesses) * (nProcesses - 1) - 1; i < height + 1; ++i) {
						for (int j = 1; j < width + 1; ++j) {
							if (strcmp(argv[k], "smooth") == 0) {
								
								float x = smoothMatrix[2][2] * red[i - 1][j - 1]
										+ smoothMatrix[2][1] * red[i - 1][j]
										+ smoothMatrix[2][0] * red[i - 1][j + 1]
										+ smoothMatrix[1][2] * red[i][j - 1]
										+ smoothMatrix[1][1] * red[i][j]
										+ smoothMatrix[1][0] * red[i][j + 1]
										+ smoothMatrix[0][2] * red[i + 1][j - 1]
										+ smoothMatrix[0][1] * red[i + 1][j]
										+ smoothMatrix[0][0] * red[i + 1][j + 1];
								
								if (x > 255.0f) {
									x = 255.0f;
								} else if (x < 0.0f) {
									x = 0.0f;
								}
								newRed[i][j] = (unsigned char) x;
								

								float y = smoothMatrix[2][2] * green[i - 1][j - 1]
										+ smoothMatrix[2][1] * green[i - 1][j]
										+ smoothMatrix[2][0] * green[i - 1][j + 1]
										+ smoothMatrix[1][2] * green[i][j - 1]
										+ smoothMatrix[1][1] * green[i][j]
										+ smoothMatrix[1][0] * green[i][j + 1]
										+ smoothMatrix[0][2] * green[i + 1][j - 1]
										+ smoothMatrix[0][1] * green[i + 1][j]
										+ smoothMatrix[0][0] * green[i + 1][j + 1];
								
								if (y > 255.0f) {
									y = 255.0f;
								} else if (y < 0.0f) {
									y = 0.0f;
								}
								newGreen[i][j] = (unsigned char) y;
							

								float z = smoothMatrix[2][2] * blue[i - 1][j - 1]
										+ smoothMatrix[2][1] * blue[i - 1][j]
										+ smoothMatrix[2][0] * blue[i - 1][j + 1]
										+ smoothMatrix[1][2] * blue[i][j - 1]
										+ smoothMatrix[1][1] * blue[i][j]
										+ smoothMatrix[1][0] * blue[i][j + 1]
										+ smoothMatrix[0][2] * blue[i + 1][j - 1]
										+ smoothMatrix[0][1] * blue[i + 1][j]
										+ smoothMatrix[0][0] * blue[i + 1][j + 1];
								
								if (z > 255.0f) {
									z = 255.0f;
								} else if (z < 0.0f) {
									z = 0.0f;
								}
								newBlue[i][j] = (unsigned char) z;
									
							} else if (strcmp(argv[k], "blur") == 0) {
								float x = blurMatrix[2][2] * red[i - 1][j - 1]
										+ blurMatrix[2][1] * red[i - 1][j]
										+ blurMatrix[2][0] * red[i - 1][j + 1]
										+ blurMatrix[1][2] * red[i][j - 1]
										+ blurMatrix[1][1] * red[i][j]
										+ blurMatrix[1][0] * red[i][j + 1]
										+ blurMatrix[0][2] * red[i + 1][j - 1]
										+ blurMatrix[0][1] * red[i + 1][j]
										+ blurMatrix[0][0] * red[i + 1][j + 1];
								
								if (x > 255.0f) {
									x = 255.0f;
								} else if (x < 0.0f) {
									x = 0.0f;
								}
								newRed[i][j] = (unsigned char) x;
								

								float y = blurMatrix[2][2] * green[i - 1][j - 1]
										+ blurMatrix[2][1] * green[i - 1][j]
										+ blurMatrix[2][0] * green[i - 1][j + 1]
										+ blurMatrix[1][2] * green[i][j - 1]
										+ blurMatrix[1][1] * green[i][j]
										+ blurMatrix[1][0] * green[i][j + 1]
										+ blurMatrix[0][2] * green[i + 1][j - 1]
										+ blurMatrix[0][1] * green[i + 1][j]
										+ blurMatrix[0][0] * green[i + 1][j + 1];
								
								if (y > 255.0f) {
									y = 255.0f;
								} else if (y < 0.0f) {
									y = 0.0f;
								}
								newGreen[i][j] = (unsigned char) y;
								

								float z = blurMatrix[2][2] * blue[i - 1][j - 1]
										+ blurMatrix[2][1] * blue[i - 1][j]
										+ blurMatrix[2][0] * blue[i - 1][j + 1]
										+ blurMatrix[1][2] * blue[i][j - 1]
										+ blurMatrix[1][1] * blue[i][j]
										+ blurMatrix[1][0] * blue[i][j + 1]
										+ blurMatrix[0][2] * blue[i + 1][j - 1]
										+ blurMatrix[0][1] * blue[i + 1][j]
										+ blurMatrix[0][0] * blue[i + 1][j + 1];
								
								if (z > 255.0f) {
									z = 255.0f;
								} else if (z < 0.0f) {
									z = 0.0f;
								}
								newBlue[i][j] = (unsigned char) z;
							} else if (strcmp(argv[k], "mean") == 0) {
								float x = meanMatrix[2][2] * red[i - 1][j - 1]
										+ meanMatrix[2][1] * red[i - 1][j]
										+ meanMatrix[2][0] * red[i - 1][j + 1]
										+ meanMatrix[1][2] * red[i][j - 1]
										+ meanMatrix[1][1] * red[i][j]
										+ meanMatrix[1][0] * red[i][j + 1]
										+ meanMatrix[0][2] * red[i + 1][j - 1]
										+ meanMatrix[0][1] * red[i + 1][j]
										+ meanMatrix[0][0] * red[i + 1][j + 1];
								
								if (x > 255.0f) {
									x = 255.0f;
								} else if (x < 0.0f) {
									x = 0.0f;
								}
								newRed[i][j] = (unsigned char) x;
								

								float y = meanMatrix[2][2] * green[i - 1][j - 1]
										+ meanMatrix[2][1] * green[i - 1][j]
										+ meanMatrix[2][0] * green[i - 1][j + 1]
										+ meanMatrix[1][2] * green[i][j - 1]
										+ meanMatrix[1][1] * green[i][j]
										+ meanMatrix[1][0] * green[i][j + 1]
										+ meanMatrix[0][2] * green[i + 1][j - 1]
										+ meanMatrix[0][1] * green[i + 1][j]
										+ meanMatrix[0][0] * green[i + 1][j + 1];
								
								if (y > 255.0f) {
									y = 255.0f;
								} else if (y < 0.0f) {
									y = 0.0f;
								}
								newGreen[i][j] = (unsigned char) y;
								

								float z = meanMatrix[2][2] * blue[i - 1][j - 1]
										+ meanMatrix[2][1] * blue[i - 1][j]
										+ meanMatrix[2][0] * blue[i - 1][j + 1]
										+ meanMatrix[1][2] * blue[i][j - 1]
										+ meanMatrix[1][1] * blue[i][j]
										+ meanMatrix[1][0] * blue[i][j + 1]
										+ meanMatrix[0][2] * blue[i + 1][j - 1]
										+ meanMatrix[0][1] * blue[i + 1][j]
										+ meanMatrix[0][0] * blue[i + 1][j + 1];
								
								if (z > 255.0f) {
									z = 255.0f;
								} else if (z < 0.0f) {
									z = 0.0f;
								}
								newBlue[i][j] = (unsigned char) z;
							} else if (strcmp(argv[k], "emboss") == 0) {
								float x = embossMatrix[2][2] * red[i - 1][j - 1]
										+ embossMatrix[2][1] * red[i - 1][j]
										+ embossMatrix[2][0] * red[i - 1][j + 1]
										+ embossMatrix[1][2] * red[i][j - 1]
										+ embossMatrix[1][1] * red[i][j]
										+ embossMatrix[1][0] * red[i][j + 1]
										+ embossMatrix[0][2] * red[i + 1][j - 1]
										+ embossMatrix[0][1] * red[i + 1][j]
										+ embossMatrix[0][0] * red[i + 1][j + 1];
								
								if (x > 255.0f) {
									x = 255.0f;
								} else if (x < 0.0f) {
									x = 0.0f;
								}
								newRed[i][j] = (unsigned char) x;
								

								float y = embossMatrix[2][2] * green[i - 1][j - 1]
										+ embossMatrix[2][1] * green[i - 1][j]
										+ embossMatrix[2][0] * green[i - 1][j + 1]
										+ embossMatrix[1][2] * green[i][j - 1]
										+ embossMatrix[1][1] * green[i][j]
										+ embossMatrix[1][0] * green[i][j + 1]
										+ embossMatrix[0][2] * green[i + 1][j - 1]
										+ embossMatrix[0][1] * green[i + 1][j]
										+ embossMatrix[0][0] * green[i + 1][j + 1];
								
								if (y > 255.0f) {
									y = 255.0f;
								} else if (y < 0.0f) {
									y = 0.0f;
								}
								newGreen[i][j] = (unsigned char) y;
								

								float z = embossMatrix[2][2] * blue[i - 1][j - 1]
										+ embossMatrix[2][1] * blue[i - 1][j]
										+ embossMatrix[2][0] * blue[i - 1][j + 1]
										+ embossMatrix[1][2] * blue[i][j - 1]
										+ embossMatrix[1][1] * blue[i][j]
										+ embossMatrix[1][0] * blue[i][j + 1]
										+ embossMatrix[0][2] * blue[i + 1][j - 1]
										+ embossMatrix[0][1] * blue[i + 1][j]
										+ embossMatrix[0][0] * blue[i + 1][j + 1];
								
								if (z > 255.0f) {
									z = 255.0f;
								} else if (z < 0.0f) {
									z = 0.0f;
								}
								newBlue[i][j] = (unsigned char) z;
							} else if (strcmp(argv[k], "sharpen") == 0) {
								float x = sharpenMatrix[2][2] * red[i - 1][j - 1]
										+ sharpenMatrix[2][1] * red[i - 1][j]
										+ sharpenMatrix[2][0] * red[i - 1][j + 1]
										+ sharpenMatrix[1][2] * red[i][j - 1]
										+ sharpenMatrix[1][1] * red[i][j]
										+ sharpenMatrix[1][0] * red[i][j + 1]
										+ sharpenMatrix[0][2] * red[i + 1][j - 1]
										+ sharpenMatrix[0][1] * red[i + 1][j]
										+ sharpenMatrix[0][0] * red[i + 1][j + 1];
								
								if (x > 255.0f) {
									x = 255.0f;
								} else if (x < 0.0f) {
									x = 0.0f;
								}
								newRed[i][j] = (unsigned char) x;
								

								float y = sharpenMatrix[2][2] * green[i - 1][j - 1]
										+ sharpenMatrix[2][1] * green[i - 1][j]
										+ sharpenMatrix[2][0] * green[i - 1][j + 1]
										+ sharpenMatrix[1][2] * green[i][j - 1]
										+ sharpenMatrix[1][1] * green[i][j]
										+ sharpenMatrix[1][0] * green[i][j + 1]
										+ sharpenMatrix[0][2] * green[i + 1][j - 1]
										+ sharpenMatrix[0][1] * green[i + 1][j]
										+ sharpenMatrix[0][0] * green[i + 1][j + 1];
								
								if (y > 255.0f) {
									y = 255.0f;
								} else if (y < 0.0f) {
									y = 0.0f;
								}
								newGreen[i][j] = (unsigned char) y;
								

								float z = sharpenMatrix[2][2] * blue[i - 1][j - 1]
										+ sharpenMatrix[2][1] * blue[i - 1][j]
										+ sharpenMatrix[2][0] * blue[i - 1][j + 1]
										+ sharpenMatrix[1][2] * blue[i][j - 1]
										+ sharpenMatrix[1][1] * blue[i][j]
										+ sharpenMatrix[1][0] * blue[i][j + 1]
										+ sharpenMatrix[0][2] * blue[i + 1][j - 1]
										+ sharpenMatrix[0][1] * blue[i + 1][j]
										+ sharpenMatrix[0][0] * blue[i + 1][j + 1];
								
								if (z > 255.0f) {
									z = 255.0f;
								} else if (z < 0.0f) {
									z = 0.0f;
								}
								newBlue[i][j] = (unsigned char) z;
							}
						}
					}

					for (int i = (height / nProcesses) * (nProcesses - 1); i < height + 1; ++i) {
						for (int j = 1; j < width + 1; ++j) {
							red[i][j] = newRed[i][j];
							green[i][j] = newGreen[i][j];
							blue[i][j] = newBlue[i][j];
						}
					}

					// recv

					for (int i = 1; i < nProcesses; ++i) {
						low = max((height / nProcesses) * (i - 1), 1) - 1;
						high = (height / nProcesses) * i + 1;
						MPI_Recv(newRed[0], width + 2, MPI_CHAR, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
						MPI_Recv(newRed[0], width + 2, MPI_CHAR, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
						MPI_Recv(newRed[0], width + 2, MPI_CHAR, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
						for (int j = low + 1; j < high; ++j) {
							MPI_Recv(red[j], width + 2, MPI_CHAR, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
							MPI_Recv(green[j], width + 2, MPI_CHAR, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
							MPI_Recv(blue[j], width + 2, MPI_CHAR, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
						}
						MPI_Recv(newRed[0], width + 2, MPI_CHAR, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
						MPI_Recv(newRed[0], width + 2, MPI_CHAR, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
						MPI_Recv(newRed[0], width + 2, MPI_CHAR, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

					}
				} else {
					for (int i = 1; i < height + 1; ++i) {
						for (int j = 1; j < width + 1; ++j) {
							if (strcmp(argv[k], "smooth") == 0) {
								
								float x = smoothMatrix[2][2] * red[i - 1][j - 1]
										+ smoothMatrix[2][1] * red[i - 1][j]
										+ smoothMatrix[2][0] * red[i - 1][j + 1]
										+ smoothMatrix[1][2] * red[i][j - 1]
										+ smoothMatrix[1][1] * red[i][j]
										+ smoothMatrix[1][0] * red[i][j + 1]
										+ smoothMatrix[0][2] * red[i + 1][j - 1]
										+ smoothMatrix[0][1] * red[i + 1][j]
										+ smoothMatrix[0][0] * red[i + 1][j + 1];
								
								if (x > 255.0f) {
									x = 255.0f;
								} else if (x < 0.0f) {
									x = 0.0f;
								}
								newRed[i][j] = (unsigned char) x;
								

								float y = smoothMatrix[2][2] * green[i - 1][j - 1]
										+ smoothMatrix[2][1] * green[i - 1][j]
										+ smoothMatrix[2][0] * green[i - 1][j + 1]
										+ smoothMatrix[1][2] * green[i][j - 1]
										+ smoothMatrix[1][1] * green[i][j]
										+ smoothMatrix[1][0] * green[i][j + 1]
										+ smoothMatrix[0][2] * green[i + 1][j - 1]
										+ smoothMatrix[0][1] * green[i + 1][j]
										+ smoothMatrix[0][0] * green[i + 1][j + 1];
								
								if (y > 255.0f) {
									y = 255.0f;
								} else if (y < 0.0f) {
									y = 0.0f;
								}
								newGreen[i][j] = (unsigned char) y;

								float z = smoothMatrix[2][2] * blue[i - 1][j - 1]
										+ smoothMatrix[2][1] * blue[i - 1][j]
										+ smoothMatrix[2][0] * blue[i - 1][j + 1]
										+ smoothMatrix[1][2] * blue[i][j - 1]
										+ smoothMatrix[1][1] * blue[i][j]
										+ smoothMatrix[1][0] * blue[i][j + 1]
										+ smoothMatrix[0][2] * blue[i + 1][j - 1]
										+ smoothMatrix[0][1] * blue[i + 1][j]
										+ smoothMatrix[0][0] * blue[i + 1][j + 1];
								
								if (z > 255.0f) {
									z = 255.0f;
								} else if (z < 0.0f) {
									z = 0.0f;
								}
								newBlue[i][j] = (unsigned char) z;
									
							} else if (strcmp(argv[k], "blur") == 0) {
								float x = blurMatrix[2][2] * red[i - 1][j - 1]
										+ blurMatrix[2][1] * red[i - 1][j]
										+ blurMatrix[2][0] * red[i - 1][j + 1]
										+ blurMatrix[1][2] * red[i][j - 1]
										+ blurMatrix[1][1] * red[i][j]
										+ blurMatrix[1][0] * red[i][j + 1]
										+ blurMatrix[0][2] * red[i + 1][j - 1]
										+ blurMatrix[0][1] * red[i + 1][j]
										+ blurMatrix[0][0] * red[i + 1][j + 1];
								
								if (x > 255.0f) {
									x = 255.0f;
								} else if (x < 0.0f) {
									x = 0.0f;
								}
								newRed[i][j] = (unsigned char) x;
								

								float y = blurMatrix[2][2] * green[i - 1][j - 1]
										+ blurMatrix[2][1] * green[i - 1][j]
										+ blurMatrix[2][0] * green[i - 1][j + 1]
										+ blurMatrix[1][2] * green[i][j - 1]
										+ blurMatrix[1][1] * green[i][j]
										+ blurMatrix[1][0] * green[i][j + 1]
										+ blurMatrix[0][2] * green[i + 1][j - 1]
										+ blurMatrix[0][1] * green[i + 1][j]
										+ blurMatrix[0][0] * green[i + 1][j + 1];
								
								if (y > 255.0f) {
									y = 255.0f;
								} else if (y < 0.0f) {
									y = 0.0f;
								}
								newGreen[i][j] = (unsigned char) y;
								

								float z = blurMatrix[2][2] * blue[i - 1][j - 1]
										+ blurMatrix[2][1] * blue[i - 1][j]
										+ blurMatrix[2][0] * blue[i - 1][j + 1]
										+ blurMatrix[1][2] * blue[i][j - 1]
										+ blurMatrix[1][1] * blue[i][j]
										+ blurMatrix[1][0] * blue[i][j + 1]
										+ blurMatrix[0][2] * blue[i + 1][j - 1]
										+ blurMatrix[0][1] * blue[i + 1][j]
										+ blurMatrix[0][0] * blue[i + 1][j + 1];
								
								if (z > 255.0f) {
									z = 255.0f;
								} else if (z < 0.0f) {
									z = 0.0f;
								}
								newBlue[i][j] = (unsigned char) z;
								
							} else if (strcmp(argv[k], "mean") == 0) {
								float x = meanMatrix[2][2] * red[i - 1][j - 1]
										+ meanMatrix[2][1] * red[i - 1][j]
										+ meanMatrix[2][0] * red[i - 1][j + 1]
										+ meanMatrix[1][2] * red[i][j - 1]
										+ meanMatrix[1][1] * red[i][j]
										+ meanMatrix[1][0] * red[i][j + 1]
										+ meanMatrix[0][2] * red[i + 1][j - 1]
										+ meanMatrix[0][1] * red[i + 1][j]
										+ meanMatrix[0][0] * red[i + 1][j + 1];
								
								if (x > 255.0f) {
									x = 255.0f;
								} else if (x < 0.0f) {
									x = 0.0f;
								}
								newRed[i][j] = (unsigned char) x;
								

								float y = meanMatrix[2][2] * green[i - 1][j - 1]
										+ meanMatrix[2][1] * green[i - 1][j]
										+ meanMatrix[2][0] * green[i - 1][j + 1]
										+ meanMatrix[1][2] * green[i][j - 1]
										+ meanMatrix[1][1] * green[i][j]
										+ meanMatrix[1][0] * green[i][j + 1]
										+ meanMatrix[0][2] * green[i + 1][j - 1]
										+ meanMatrix[0][1] * green[i + 1][j]
										+ meanMatrix[0][0] * green[i + 1][j + 1];
								
								if (y > 255.0f) {
									y = 255.0f;
								} else if (y < 0.0f) {
									y = 0.0f;
								}
								newGreen[i][j] = (unsigned char) y;
								

								float z = meanMatrix[2][2] * blue[i - 1][j - 1]
										+ meanMatrix[2][1] * blue[i - 1][j]
										+ meanMatrix[2][0] * blue[i - 1][j + 1]
										+ meanMatrix[1][2] * blue[i][j - 1]
										+ meanMatrix[1][1] * blue[i][j]
										+ meanMatrix[1][0] * blue[i][j + 1]
										+ meanMatrix[0][2] * blue[i + 1][j - 1]
										+ meanMatrix[0][1] * blue[i + 1][j]
										+ meanMatrix[0][0] * blue[i + 1][j + 1];
								
								if (z > 255.0f) {
									z = 255.0f;
								} else if (z < 0.0f) {
									z = 0.0f;
								}
								newBlue[i][j] = (unsigned char) z;
								
							} else if (strcmp(argv[k], "emboss") == 0) {
								float x = embossMatrix[2][2] * red[i - 1][j - 1]
										+ embossMatrix[2][1] * red[i - 1][j]
										+ embossMatrix[2][0] * red[i - 1][j + 1]
										+ embossMatrix[1][2] * red[i][j - 1]
										+ embossMatrix[1][1] * red[i][j]
										+ embossMatrix[1][0] * red[i][j + 1]
										+ embossMatrix[0][2] * red[i + 1][j - 1]
										+ embossMatrix[0][1] * red[i + 1][j]
										+ embossMatrix[0][0] * red[i + 1][j + 1];
								
								if (x > 255.0f) {
									x = 255.0f;
								} else if (x < 0.0f) {
									x = 0.0f;
								}
								newRed[i][j] = (unsigned char) x;
								

								float y = embossMatrix[2][2] * green[i - 1][j - 1]
										+ embossMatrix[2][1] * green[i - 1][j]
										+ embossMatrix[2][0] * green[i - 1][j + 1]
										+ embossMatrix[1][2] * green[i][j - 1]
										+ embossMatrix[1][1] * green[i][j]
										+ embossMatrix[1][0] * green[i][j + 1]
										+ embossMatrix[0][2] * green[i + 1][j - 1]
										+ embossMatrix[0][1] * green[i + 1][j]
										+ embossMatrix[0][0] * green[i + 1][j + 1];
								
								if (y > 255.0f) {
									y = 255.0f;
								} else if (y < 0.0f) {
									y = 0.0f;
								}
								newGreen[i][j] = (unsigned char) y;
								

								float z = embossMatrix[2][2] * blue[i - 1][j - 1]
										+ embossMatrix[2][1] * blue[i - 1][j]
										+ embossMatrix[2][0] * blue[i - 1][j + 1]
										+ embossMatrix[1][2] * blue[i][j - 1]
										+ embossMatrix[1][1] * blue[i][j]
										+ embossMatrix[1][0] * blue[i][j + 1]
										+ embossMatrix[0][2] * blue[i + 1][j - 1]
										+ embossMatrix[0][1] * blue[i + 1][j]
										+ embossMatrix[0][0] * blue[i + 1][j + 1];
								
								if (z > 255.0f) {
									z = 255.0f;
								} else if (z < 0.0f) {
									z = 0.0f;
								}
								newBlue[i][j] = (unsigned char) z;
								
							} else if (strcmp(argv[k], "sharpen") == 0) {
								float x = sharpenMatrix[2][2] * red[i - 1][j - 1]
										+ sharpenMatrix[2][1] * red[i - 1][j]
										+ sharpenMatrix[2][0] * red[i - 1][j + 1]
										+ sharpenMatrix[1][2] * red[i][j - 1]
										+ sharpenMatrix[1][1] * red[i][j]
										+ sharpenMatrix[1][0] * red[i][j + 1]
										+ sharpenMatrix[0][2] * red[i + 1][j - 1]
										+ sharpenMatrix[0][1] * red[i + 1][j]
										+ sharpenMatrix[0][0] * red[i + 1][j + 1];
								
								if (x > 255.0f) {
									x = 255.0f;
								} else if (x < 0.0f) {
									x = 0.0f;
								}
								newRed[i][j] = (unsigned char) x;
								

								float y = sharpenMatrix[2][2] * green[i - 1][j - 1]
										+ sharpenMatrix[2][1] * green[i - 1][j]
										+ sharpenMatrix[2][0] * green[i - 1][j + 1]
										+ sharpenMatrix[1][2] * green[i][j - 1]
										+ sharpenMatrix[1][1] * green[i][j]
										+ sharpenMatrix[1][0] * green[i][j + 1]
										+ sharpenMatrix[0][2] * green[i + 1][j - 1]
										+ sharpenMatrix[0][1] * green[i + 1][j]
										+ sharpenMatrix[0][0] * green[i + 1][j + 1];
								
								if (y > 255.0f) {
									y = 255.0f;
								} else if (y < 0.0f) {
									y = 0.0f;
								}
								newGreen[i][j] = (unsigned char) y;
								

								float z = sharpenMatrix[2][2] * blue[i - 1][j - 1]
										+ sharpenMatrix[2][1] * blue[i - 1][j]
										+ sharpenMatrix[2][0] * blue[i - 1][j + 1]
										+ sharpenMatrix[1][2] * blue[i][j - 1]
										+ sharpenMatrix[1][1] * blue[i][j]
										+ sharpenMatrix[1][0] * blue[i][j + 1]
										+ sharpenMatrix[0][2] * blue[i + 1][j - 1]
										+ sharpenMatrix[0][1] * blue[i + 1][j]
										+ sharpenMatrix[0][0] * blue[i + 1][j + 1];
								
								if (z > 255.0f) {
									z = 255.0f;
								} else if (z < 0.0f) {
									z = 0.0f;
								}
								newBlue[i][j] = (unsigned char) z;
								
							}
						}
					}

					for (int i = 1; i < height + 1; ++i) {
						for (int j = 1; j < width + 1; ++j) {
							red[i][j] = newRed[i][j];
							green[i][j] = newGreen[i][j];
							blue[i][j] = newBlue[i][j];
						}
					}
				}
				k++;
			}

			// scriere fisier
			printf("Pt %d: low = %d   high = %d\n", rank, (height/nProcesses) * (nProcesses - 1) - 1, height + 1);
			FILE *out;
			out = fopen(argv[2], "w");
			char ch = 'P';
			fprintf(out, "%c", ch);
			fprintf(out, "%d\n", P);
			fprintf(out, "%s", s1);
			free(s1);
			fprintf(out, "%d %d\n", width, height);
			fprintf(out, "%d\n", maxValue);

			for (int i = 1; i < height + 1; ++i) {
				for (int j = 1; j < width + 1; ++j) {
					c = red[i][j];
					fprintf(out, "%c", c);
					c = green[i][j];
					fprintf(out, "%c", c);
					c = blue[i][j];
					fprintf(out, "%c", c);
				}
			}
			fclose(out);
		}

	    fclose(f);

	} else {

		int P;
		int height;
		int width;
		int nrLines;
		if (nProcesses != 1) {
			MPI_Recv(&P, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			MPI_Recv(&height, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			MPI_Recv(&width, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			MPI_Recv(&nrLines, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			
			if (P == 5) {
				unsigned char** blackWhite;
				unsigned char** newBW;
				blackWhite = (unsigned char **) malloc ((nrLines) * sizeof(unsigned char*));
				newBW = (unsigned char **) malloc ((nrLines) * sizeof(unsigned char*));

				for (int i = 0; i < nrLines; ++i) {
					blackWhite[i] = (unsigned char*) malloc ((width + 2) * sizeof(unsigned char));
					newBW[i] = (unsigned char*) malloc ((width + 2) * sizeof(unsigned char));
				}

				int k = 3;
				while (k < argc) {
					for (int j = 0; j < nrLines; ++j) {
						MPI_Recv(blackWhite[j], width + 2, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
					}

					// calcul
					for (int i = 1; i < nrLines - 1; ++i) {
						for (int j = 1; j < width + 1; ++j) {
							if (strcmp(argv[k], "smooth") == 0) {
								
								float x = smoothMatrix[2][2] * blackWhite[i - 1][j - 1]
										+ smoothMatrix[2][1] * blackWhite[i - 1][j]
										+ smoothMatrix[2][0] * blackWhite[i - 1][j + 1]
										+ smoothMatrix[1][2] * blackWhite[i][j - 1]
										+ smoothMatrix[1][1] * blackWhite[i][j]
										+ smoothMatrix[1][0] * blackWhite[i][j + 1]
										+ smoothMatrix[0][2] * blackWhite[i + 1][j - 1]
										+ smoothMatrix[0][1] * blackWhite[i + 1][j]
										+ smoothMatrix[0][0] * blackWhite[i + 1][j + 1];
								
								if (x > 255.0f) {
									x = 255.0f;
								} else if (x < 0.0f) {
									x = 0.0f;
								}
								newBW[i][j] = (unsigned char) x;
								
									
							} else if (strcmp(argv[k], "blur") == 0) {
								float x = blurMatrix[2][2] * blackWhite[i - 1][j - 1]
										+ blurMatrix[2][1] * blackWhite[i - 1][j]
										+ blurMatrix[2][0] * blackWhite[i - 1][j + 1]
										+ blurMatrix[1][2] * blackWhite[i][j - 1]
										+ blurMatrix[1][1] * blackWhite[i][j]
										+ blurMatrix[1][0] * blackWhite[i][j + 1]
										+ blurMatrix[0][2] * blackWhite[i + 1][j - 1]
										+ blurMatrix[0][1] * blackWhite[i + 1][j]
										+ blurMatrix[0][0] * blackWhite[i + 1][j + 1];
								
								if (x > 255.0f) {
									x = 255.0f;
								} else if (x < 0.0f) {
									x = 0.0f;
								}
								newBW[i][j] = (unsigned char) x;
								
							} else if (strcmp(argv[k], "mean") == 0) {
								float x = meanMatrix[2][2] * blackWhite[i - 1][j - 1]
										+ meanMatrix[2][1] * blackWhite[i - 1][j]
										+ meanMatrix[2][0] * blackWhite[i - 1][j + 1]
										+ meanMatrix[1][2] * blackWhite[i][j - 1]
										+ meanMatrix[1][1] * blackWhite[i][j]
										+ meanMatrix[1][0] * blackWhite[i][j + 1]
										+ meanMatrix[0][2] * blackWhite[i + 1][j - 1]
										+ meanMatrix[0][1] * blackWhite[i + 1][j]
										+ meanMatrix[0][0] * blackWhite[i + 1][j + 1];
								
								if (x > 255.0f) {
									x = 255.0f;
								} else if (x < 0.0f) {
									x = 0.0f;
								}
								newBW[i][j] = (unsigned char) x;
								
							} else if (strcmp(argv[k], "emboss") == 0) {
								float x = embossMatrix[2][2] * blackWhite[i - 1][j - 1]
										+ embossMatrix[2][1] * blackWhite[i - 1][j]
										+ embossMatrix[2][0] * blackWhite[i - 1][j + 1]
										+ embossMatrix[1][2] * blackWhite[i][j - 1]
										+ embossMatrix[1][1] * blackWhite[i][j]
										+ embossMatrix[1][0] * blackWhite[i][j + 1]
										+ embossMatrix[0][2] * blackWhite[i + 1][j - 1]
										+ embossMatrix[0][1] * blackWhite[i + 1][j]
										+ embossMatrix[0][0] * blackWhite[i + 1][j + 1];
								
								if (x > 255.0f) {
									x = 255.0f;
								} else if (x < 0.0f) {
									x = 0.0f;
								}
								newBW[i][j] = (unsigned char) x;

							} else if (strcmp(argv[k], "sharpen") == 0) {
								float x = sharpenMatrix[2][2] * blackWhite[i - 1][j - 1]
										+ sharpenMatrix[2][1] * blackWhite[i - 1][j]
										+ sharpenMatrix[2][0] * blackWhite[i - 1][j + 1]
										+ sharpenMatrix[1][2] * blackWhite[i][j - 1]
										+ sharpenMatrix[1][1] * blackWhite[i][j]
										+ sharpenMatrix[1][0] * blackWhite[i][j + 1]
										+ sharpenMatrix[0][2] * blackWhite[i + 1][j - 1]
										+ sharpenMatrix[0][1] * blackWhite[i + 1][j]
										+ sharpenMatrix[0][0] * blackWhite[i + 1][j + 1];
								
								if (x > 255.0f) {
									x = 255.0f;
								} else if (x < 0.0f) {
									x = 0.0f;
								}
								newBW[i][j] = (unsigned char) x;
							}
						}
					}

					for (int i = 0; i < nrLines; ++i) {
						for (int j = 1; j < width + 1; ++j) {
							blackWhite[i][j] = newBW[i][j];
						}
					}
					// send
					for (int j = 0; j < nrLines; ++j) {
						MPI_Send(blackWhite[j], width + 2, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
					}
					k++;
				}
			} else {
				unsigned char** red;
				unsigned char** green;
				unsigned char** blue;

				red = (unsigned char **) malloc ((nrLines) * sizeof(unsigned char*));
				green = (unsigned char **) malloc ((nrLines) * sizeof(unsigned char*));
				blue = (unsigned char **) malloc ((nrLines) * sizeof(unsigned char*));

				for (int i = 0; i < nrLines; ++i) {
					red[i] = (unsigned char*) malloc ((width + 2) * sizeof(unsigned char));
					green[i] = (unsigned char*) malloc ((width + 2) * sizeof(unsigned char));
					blue[i] = (unsigned char*) malloc ((width + 2) * sizeof(unsigned char));
				}

				unsigned char** newRed;
				unsigned char** newGreen;
				unsigned char** newBlue;

				newRed = (unsigned char **) malloc ((nrLines) * sizeof(unsigned char*));
				newGreen = (unsigned char **) malloc ((nrLines) * sizeof(unsigned char*));
				newBlue = (unsigned char **) malloc ((nrLines) * sizeof(unsigned char*));

				for (int i = 0; i < nrLines; ++i) {
					newRed[i] = (unsigned char*) malloc ((width + 2) * sizeof(unsigned char));
					newGreen[i] = (unsigned char*) malloc ((width + 2) * sizeof(unsigned char));
					newBlue[i] = (unsigned char*) malloc ((width + 2) * sizeof(unsigned char));
				}
				int k = 3;
				while (k < argc) {
					for (int j = 0; j < nrLines; ++j) {
						MPI_Recv(red[j], width + 2, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
						MPI_Recv(green[j], width + 2, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
						MPI_Recv(blue[j], width + 2, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
					}

					// calcul
					for (int i = 1; i < nrLines - 1; ++i) {
						for (int j = 1; j < width + 1; ++j) {
							if (strcmp(argv[k], "smooth") == 0) {
								
								float x = smoothMatrix[2][2] * red[i - 1][j - 1]
										+ smoothMatrix[2][1] * red[i - 1][j]
										+ smoothMatrix[2][0] * red[i - 1][j + 1]
										+ smoothMatrix[1][2] * red[i][j - 1]
										+ smoothMatrix[1][1] * red[i][j]
										+ smoothMatrix[1][0] * red[i][j + 1]
										+ smoothMatrix[0][2] * red[i + 1][j - 1]
										+ smoothMatrix[0][1] * red[i + 1][j]
										+ smoothMatrix[0][0] * red[i + 1][j + 1];
								
								if (x > 255.0f) {
									x = 255.0f;
								} else if (x < 0.0f) {
									x = 0.0f;
								}
								newRed[i][j] = (unsigned char) x;
								

								float y = smoothMatrix[2][2] * green[i - 1][j - 1]
										+ smoothMatrix[2][1] * green[i - 1][j]
										+ smoothMatrix[2][0] * green[i - 1][j + 1]
										+ smoothMatrix[1][2] * green[i][j - 1]
										+ smoothMatrix[1][1] * green[i][j]
										+ smoothMatrix[1][0] * green[i][j + 1]
										+ smoothMatrix[0][2] * green[i + 1][j - 1]
										+ smoothMatrix[0][1] * green[i + 1][j]
										+ smoothMatrix[0][0] * green[i + 1][j + 1];
								
								if (y > 255.0f) {
									y = 255.0f;
								} else if (y < 0.0f) {
									y = 0.0f;
								}
								newGreen[i][j] = (unsigned char) y;
								

								float z = smoothMatrix[2][2] * blue[i - 1][j - 1]
										+ smoothMatrix[2][1] * blue[i - 1][j]
										+ smoothMatrix[2][0] * blue[i - 1][j + 1]
										+ smoothMatrix[1][2] * blue[i][j - 1]
										+ smoothMatrix[1][1] * blue[i][j]
										+ smoothMatrix[1][0] * blue[i][j + 1]
										+ smoothMatrix[0][2] * blue[i + 1][j - 1]
										+ smoothMatrix[0][1] * blue[i + 1][j]
										+ smoothMatrix[0][0] * blue[i + 1][j + 1];
								
								if (z > 255.0f) {
									z = 255.0f;
								} else if (z < 0.0f) {
									z = 0.0f;
								}
								newBlue[i][j] = (unsigned char) z;
									
							} else if (strcmp(argv[k], "blur") == 0) {
								float x = blurMatrix[2][2] * red[i - 1][j - 1]
										+ blurMatrix[2][1] * red[i - 1][j]
										+ blurMatrix[2][0] * red[i - 1][j + 1]
										+ blurMatrix[1][2] * red[i][j - 1]
										+ blurMatrix[1][1] * red[i][j]
										+ blurMatrix[1][0] * red[i][j + 1]
										+ blurMatrix[0][2] * red[i + 1][j - 1]
										+ blurMatrix[0][1] * red[i + 1][j]
										+ blurMatrix[0][0] * red[i + 1][j + 1];
								
								if (x > 255.0f) {
									x = 255.0f;
								} else if (x < 0.0f) {
									x = 0.0f;
								}
								newRed[i][j] = (unsigned char) x;
								

								float y = blurMatrix[2][2] * green[i - 1][j - 1]
										+ blurMatrix[2][1] * green[i - 1][j]
										+ blurMatrix[2][0] * green[i - 1][j + 1]
										+ blurMatrix[1][2] * green[i][j - 1]
										+ blurMatrix[1][1] * green[i][j]
										+ blurMatrix[1][0] * green[i][j + 1]
										+ blurMatrix[0][2] * green[i + 1][j - 1]
										+ blurMatrix[0][1] * green[i + 1][j]
										+ blurMatrix[0][0] * green[i + 1][j + 1];
								
								if (y > 255.0f) {
									y = 255.0f;
								} else if (y < 0.0f) {
									y = 0.0f;
								}
								newGreen[i][j] = (unsigned char) y;
								

								float z = blurMatrix[2][2] * blue[i - 1][j - 1]
										+ blurMatrix[2][1] * blue[i - 1][j]
										+ blurMatrix[2][0] * blue[i - 1][j + 1]
										+ blurMatrix[1][2] * blue[i][j - 1]
										+ blurMatrix[1][1] * blue[i][j]
										+ blurMatrix[1][0] * blue[i][j + 1]
										+ blurMatrix[0][2] * blue[i + 1][j - 1]
										+ blurMatrix[0][1] * blue[i + 1][j]
										+ blurMatrix[0][0] * blue[i + 1][j + 1];
								
								if (z > 255.0f) {
									z = 255.0f;
								} else if (z < 0.0f) {
									z = 0.0f;
								}
								newBlue[i][j] = (unsigned char) z;
							} else if (strcmp(argv[k], "mean") == 0) {
								float x = meanMatrix[2][2] * red[i - 1][j - 1]
										+ meanMatrix[2][1] * red[i - 1][j]
										+ meanMatrix[2][0] * red[i - 1][j + 1]
										+ meanMatrix[1][2] * red[i][j - 1]
										+ meanMatrix[1][1] * red[i][j]
										+ meanMatrix[1][0] * red[i][j + 1]
										+ meanMatrix[0][2] * red[i + 1][j - 1]
										+ meanMatrix[0][1] * red[i + 1][j]
										+ meanMatrix[0][0] * red[i + 1][j + 1];
								
								if (x > 255.0f) {
									x = 255.0f;
								} else if (x < 0.0f) {
									x = 0.0f;
								}
								newRed[i][j] = (unsigned char) x;
								

								float y = meanMatrix[2][2] * green[i - 1][j - 1]
										+ meanMatrix[2][1] * green[i - 1][j]
										+ meanMatrix[2][0] * green[i - 1][j + 1]
										+ meanMatrix[1][2] * green[i][j - 1]
										+ meanMatrix[1][1] * green[i][j]
										+ meanMatrix[1][0] * green[i][j + 1]
										+ meanMatrix[0][2] * green[i + 1][j - 1]
										+ meanMatrix[0][1] * green[i + 1][j]
										+ meanMatrix[0][0] * green[i + 1][j + 1];
								
								if (y > 255.0f) {
									y = 255.0f;
								} else if (y < 0.0f) {
									y = 0.0f;
								}
								newGreen[i][j] = (unsigned char) y;
								

								float z = meanMatrix[2][2] * blue[i - 1][j - 1]
										+ meanMatrix[2][1] * blue[i - 1][j]
										+ meanMatrix[2][0] * blue[i - 1][j + 1]
										+ meanMatrix[1][2] * blue[i][j - 1]
										+ meanMatrix[1][1] * blue[i][j]
										+ meanMatrix[1][0] * blue[i][j + 1]
										+ meanMatrix[0][2] * blue[i + 1][j - 1]
										+ meanMatrix[0][1] * blue[i + 1][j]
										+ meanMatrix[0][0] * blue[i + 1][j + 1];
								
								if (z > 255.0f) {
									z = 255.0f;
								} else if (z < 0.0f) {
									z = 0.0f;
								}
								newBlue[i][j] = (unsigned char) z;
							} else if (strcmp(argv[k], "emboss") == 0) {
								float x = embossMatrix[2][2] * red[i - 1][j - 1]
										+ embossMatrix[2][1] * red[i - 1][j]
										+ embossMatrix[2][0] * red[i - 1][j + 1]
										+ embossMatrix[1][2] * red[i][j - 1]
										+ embossMatrix[1][1] * red[i][j]
										+ embossMatrix[1][0] * red[i][j + 1]
										+ embossMatrix[0][2] * red[i + 1][j - 1]
										+ embossMatrix[0][1] * red[i + 1][j]
										+ embossMatrix[0][0] * red[i + 1][j + 1];
								
								if (x > 255.0f) {
									x = 255.0f;
								} else if (x < 0.0f) {
									x = 0.0f;
								}
								newRed[i][j] = (unsigned char) x;
								

								float y = embossMatrix[2][2] * green[i - 1][j - 1]
										+ embossMatrix[2][1] * green[i - 1][j]
										+ embossMatrix[2][0] * green[i - 1][j + 1]
										+ embossMatrix[1][2] * green[i][j - 1]
										+ embossMatrix[1][1] * green[i][j]
										+ embossMatrix[1][0] * green[i][j + 1]
										+ embossMatrix[0][2] * green[i + 1][j - 1]
										+ embossMatrix[0][1] * green[i + 1][j]
										+ embossMatrix[0][0] * green[i + 1][j + 1];
								
								if (y > 255.0f) {
									y = 255.0f;
								} else if (y < 0.0f) {
									y = 0.0f;
								}
								newGreen[i][j] = (unsigned char) y;
								

								float z = embossMatrix[2][2] * blue[i - 1][j - 1]
										+ embossMatrix[2][1] * blue[i - 1][j]
										+ embossMatrix[2][0] * blue[i - 1][j + 1]
										+ embossMatrix[1][2] * blue[i][j - 1]
										+ embossMatrix[1][1] * blue[i][j]
										+ embossMatrix[1][0] * blue[i][j + 1]
										+ embossMatrix[0][2] * blue[i + 1][j - 1]
										+ embossMatrix[0][1] * blue[i + 1][j]
										+ embossMatrix[0][0] * blue[i + 1][j + 1];
								
								if (z > 255.0f) {
									z = 255.0f;
								} else if (z < 0.0f) {
									z = 0.0f;
								}
								newBlue[i][j] = (unsigned char) z;
							} else if (strcmp(argv[k], "sharpen") == 0) {
								float x = sharpenMatrix[2][2] * red[i - 1][j - 1]
										+ sharpenMatrix[2][1] * red[i - 1][j]
										+ sharpenMatrix[2][0] * red[i - 1][j + 1]
										+ sharpenMatrix[1][2] * red[i][j - 1]
										+ sharpenMatrix[1][1] * red[i][j]
										+ sharpenMatrix[1][0] * red[i][j + 1]
										+ sharpenMatrix[0][2] * red[i + 1][j - 1]
										+ sharpenMatrix[0][1] * red[i + 1][j]
										+ sharpenMatrix[0][0] * red[i + 1][j + 1];
								
								if (x > 255.0f) {
									x = 255.0f;
								} else if (x < 0.0f) {
									x = 0.0f;
								}
								newRed[i][j] = (unsigned char) x;
								

								float y = sharpenMatrix[2][2] * green[i - 1][j - 1]
										+ sharpenMatrix[2][1] * green[i - 1][j]
										+ sharpenMatrix[2][0] * green[i - 1][j + 1]
										+ sharpenMatrix[1][2] * green[i][j - 1]
										+ sharpenMatrix[1][1] * green[i][j]
										+ sharpenMatrix[1][0] * green[i][j + 1]
										+ sharpenMatrix[0][2] * green[i + 1][j - 1]
										+ sharpenMatrix[0][1] * green[i + 1][j]
										+ sharpenMatrix[0][0] * green[i + 1][j + 1];
								
								if (y > 255.0f) {
									y = 255.0f;
								} else if (y < 0.0f) {
									y = 0.0f;
								}
								newGreen[i][j] = (unsigned char) y;
								

								float z = sharpenMatrix[2][2] * blue[i - 1][j - 1]
										+ sharpenMatrix[2][1] * blue[i - 1][j]
										+ sharpenMatrix[2][0] * blue[i - 1][j + 1]
										+ sharpenMatrix[1][2] * blue[i][j - 1]
										+ sharpenMatrix[1][1] * blue[i][j]
										+ sharpenMatrix[1][0] * blue[i][j + 1]
										+ sharpenMatrix[0][2] * blue[i + 1][j - 1]
										+ sharpenMatrix[0][1] * blue[i + 1][j]
										+ sharpenMatrix[0][0] * blue[i + 1][j + 1];
								
								if (z > 255.0f) {
									z = 255.0f;
								} else if (z < 0.0f) {
									z = 0.0f;
								}
								newBlue[i][j] = (unsigned char) z;
							}
						}
					}

					for (int i = 0; i < nrLines; ++i) {
						for (int j = 1; j < width + 1; ++j) {
							red[i][j] = newRed[i][j];
							green[i][j] = newGreen[i][j];
							blue[i][j] = newBlue[i][j];
						}
					}
					// send
					for (int j = 0; j < nrLines; ++j) {
						MPI_Send(red[j], width + 2, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
						MPI_Send(green[j], width + 2, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
						MPI_Send(blue[j], width + 2, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
					}
					k++;
				}

			}
		}
	}

	MPI_Finalize();
	return 0;
}