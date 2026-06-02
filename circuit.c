#include <stdio.h>
#include <raylib.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>

// wire stuff
#define N_E 2000
#define V 25
#define R_CU 6
#define R_E 2
#define GAP 8
#define THICCNESS 120
#define STEP (2*R_CU + GAP) // 20
#define D_CU (2*R_CU) // 12

// screen stuff, assuming 1920 x 1080
#define OUTER_WIDTH 1720 
#define OUTER_HEIGHT 880 
#define INNER_WIDTH (OUTER_WIDTH - 2*THICCNESS)   // 1720 - 240 = 1480
#define INNER_HEIGHT (OUTER_HEIGHT - 2*THICCNESS) // 880 - 240 = 640
#define OFFSET 100

Vector2 e[N_E];
Vector2 v[N_E];
Vector2 a[N_E] = {0};

// atom lattice
#define LR_COLS (THICCNESS/(2*R_CU + GAP))
#define LR_ROWS (OUTER_HEIGHT/(2*R_CU + GAP)) 
#define TB_COLS (INNER_WIDTH/(2*R_CU + GAP))
#define TB_ROWS (THICCNESS/(2*R_CU + GAP)) 

Vector2 top[TB_ROWS][TB_COLS];
Vector2 bottom[TB_ROWS][TB_COLS];
Vector2 left[LR_ROWS][LR_COLS];
Vector2 right[LR_ROWS][LR_COLS];

// wires, battery
Rectangle outer_wire = {OFFSET, OFFSET, OUTER_WIDTH, OUTER_HEIGHT};
Rectangle inner_wire = {OFFSET + THICCNESS, OFFSET + THICCNESS, INNER_WIDTH, INNER_HEIGHT};

//E field

Vector2 E[1080][1920] = {0};
#define E_field 10

// Initialize

void Init_e() {

	for (int i=0;i<N_E;i++) {
	
		e[i].x = rand()%(350) + OFFSET + THICCNESS + INNER_WIDTH/2;
		e[i].y = rand()%(THICCNESS - 2*GAP) + OFFSET + THICCNESS + INNER_HEIGHT + 1.5f*GAP;	
	}	

	for (int i=0;i<N_E;i++) {
		v[i].x = V;
		v[i].y = -10;
	}

	for (int i=0; i<N_E; i++) {
		a[i].x = E_field*E[(int)e[i].y][(int)e[i].x].x;
		a[i].y = E_field*E[(int)e[i].y][(int)e[i].x].y;
	}
}

void Init_Cu() {
    
    for (int i=0;i<TB_ROWS;i++) {
        
        for (int j=0;j<TB_COLS;j++) {
            
            top[i][j].x = OFFSET + THICCNESS + GAP + j*(2*R_CU + GAP);
            top[i][j].y = OFFSET + GAP + i*(2*R_CU + GAP);
        }    
    }   

    for (int i=0;i<TB_ROWS;i++) {

        for (int j=0;j<TB_COLS;j++) {
            
            bottom[i][j].x = OFFSET + THICCNESS + GAP + j*(2*R_CU + GAP);
            bottom[i][j].y = OFFSET + THICCNESS + INNER_HEIGHT + GAP + i*(2*R_CU + GAP);
        }    
    }

	for (int i=0;i<LR_ROWS;i++) {

		for (int j=0;j<LR_COLS;j++) {

			left[i][j].x = OFFSET + GAP + j*(2*R_CU + GAP);
			left[i][j].y = OFFSET + GAP + i*(2*R_CU + GAP); 		
		}		
	}
	
	for (int i=0;i<LR_ROWS;i++) {

		for (int j=0;j<LR_COLS;j++) {

			right[i][j].x = OFFSET + THICCNESS + INNER_WIDTH + GAP + j*(2*R_CU + GAP);
			right[i][j].y = OFFSET + GAP + i*(2*R_CU + GAP); 				
		}		
	}

}

/*
__________________________________________________________________________________

 Electric Field

Since all moving things are electrons and all stationary things are the protons and the battery, I need to initialize an
Electric field at every single point due to only the battery and the Cu atoms is more or less constant in both magnitude and direction.

Superimposed on this is the electric field due to all the electrons, which ywould probably be very inefficient to calculate so I think instead we'll use a different method, maybe give the wire it's own field
+ ensure a max no. of electrons are in contact with the wire at any given time
*/

void Compute_E() {

	for (int tbx=0;tbx<INNER_WIDTH;tbx++) {

		for (int tby=0; tby<THICCNESS; tby++) {

			//top
			E[OFFSET + tby][OFFSET + THICCNESS + tbx].x -= E_field;
			//E[OFFSET + tby][OFFSET + THICCNESS + tbx].y += 0.2*(tby - THICCNESS/2);

			//bottom
			E[1080 - OFFSET - THICCNESS + tby][OFFSET + THICCNESS + tbx].x += E_field;
			//E[OFFSET + THICCNESS + INNER_HEIGHT + tby][OFFSET + THICCNESS + tbx].y = 0.2*(tby - THICCNESS/2);		
		}
	}

	for (int lrx=0; lrx<THICCNESS; lrx++) {

		for (int lry=0; lry<OUTER_HEIGHT; lry++) {

			// left
			//E[OFFSET + THICCNESS + lry][OFFSET + lrx].x += 0.2*(lrx - THICCNESS/2);
			E[OFFSET + lry][OFFSET + lrx].y += E_field;

			// right
			//E[OFFSET + THICCNESS + lry][OFFSET + THICCNESS + INNER_WIDTH + lrx].x = 0.2*(lrx - THICCNESS/2);
			E[OFFSET + lry][1920 - OFFSET - THICCNESS + lrx].y -= E_field;		
		}
	}	
}



// Collision logic:

int collided_with_cu = 0;
int collided_with_inner_wire = 0;
int collided_with_outer_wire = 0;

double ambm_dist(int ex, int ey, int cu_x, int cu_y) {		// alpha max beta min dist trick

	float alpha = 1.0f;
	float beta = 0.3f;

	return alpha*fmax(abs(ex - cu_x), abs(ey - cu_y)) + beta*fmin(abs(ex-cu_x),abs(ey-cu_y)); 
}

/* Okay so now that we have a neat distance formula, we have to find a way around not checking the distance against every damn copper atom in
that particular wire section, we need to find the copper atoms that are near it. What we have to work with is a matrix with the centers of 
the copper atoms in the order that they actually appear on screen, so top[i][j] and top[i][j+1] do indeed represent the positions of  two copper 
atoms that are horizontally adjacent. Okay.

We will know the coordinates of each electron. Zooming in on one electron, we only need to heck with the nearest atoms, or we need to only consider
those atoms with centers within a certain octogonal distance from the electron, this octogonal distance should be around R_CU, I think just setting
it to R_CU is good enough.

Now what we absolutely will not do is loop through the entire Cu matrix to check which ones have octogonal_dist < R_CU, way too time consuming
what we instead will do is to generate the positions from the electron position itself. We know that the centres are in an arithmetic progression

So we have equations of the centers, we need some way to find which of those points lie within the octogonal_dist region.

hm, if we look at bottom:

bottom[i][j].x = 228 + 20*j = 8 mod 20
bottom[i][j].y = 868 + 20*j also 8 mod 20

So if we are at {ex, ey}, we just want to find the nearest {x,y} such that {x,y} = {8,8} (mod 20) which can be done by just considering
{ex,ey} mod 20, we'll get some numbers that will be either more or less than 8, I think we just go for the 4 nearest {x,y} such that they
 are both 8 mod 20.

We get those by either adding or subtracting from (ex,ey) until we reach 8 mod 20.
*/

struct NearbyAtoms {
    Vector2 points[4];
};

struct NearbyAtoms NearestCu(int ex, int ey) {

    struct NearbyAtoms coords;

    float x_offset = 0;
    float y_offset = 0;

    if (ex < OFFSET + THICCNESS) {

        x_offset = OFFSET;
        y_offset = OFFSET;
    } 
    else if (ex > OFFSET + THICCNESS + INNER_WIDTH) {

        x_offset = OFFSET + THICCNESS + INNER_WIDTH;
        y_offset = OFFSET;
    } 
    else if (ey < OFFSET + THICCNESS) {
 
        x_offset = OFFSET + THICCNESS;
        y_offset = OFFSET;
    } 
    else {
        x_offset = OFFSET + THICCNESS;
        y_offset = OFFSET + THICCNESS + INNER_HEIGHT;
    }

    float x_low = STEP * ((int)(ex - x_offset - GAP) / STEP) + x_offset + GAP;
    float y_low = STEP * ((int)(ey - y_offset - GAP) / STEP) + y_offset + GAP;
    
    float x_high = x_low + STEP;
    float y_high = y_low + STEP;

    coords.points[0] = (Vector2){x_low,  y_low};
    coords.points[1] = (Vector2){x_low,  y_high};
    coords.points[2] = (Vector2){x_high, y_high};
    coords.points[3] = (Vector2){x_high, y_low};
    
    return coords;
}

Vector2 FindCollidingCu(int ex, int ey) {

    struct NearbyAtoms nearby = NearestCu(ex, ey);

    for (int i=0; i<4; i++) {
    
        int cx = (int)nearby.points[i].x;
        int cy = (int)nearby.points[i].y;

        if (ambm_dist(ex, ey, cx, cy) <= (R_CU + R_E)) {
        	collided_with_cu = 1;
            return nearby.points[i]; 
        }
    }
    
    collided_with_cu = 0;
    return (Vector2){-1.0f, -1.0f};
}

void CheckCollisionWithWire(int ex, int ey) {

    if ((ex >= OFFSET + THICCNESS - R_E) && (ex <= 1920 - (THICCNESS + OFFSET) + R_E) && (ey >= OFFSET + THICCNESS - R_E) && (ey <= 1080 - (OFFSET + THICCNESS) + R_E)) {
        collided_with_inner_wire = 1;
        collided_with_outer_wire = 0;
    }

    else if ((ex <= OFFSET + R_E) || (ex >= 1920 - OFFSET - R_E) || (ey <= OFFSET + R_E) || (ey >= 1080 - OFFSET - R_E)) {
        collided_with_outer_wire = 1;
        collided_with_inner_wire = 0;
    }
    else {
        collided_with_inner_wire = 0;
        collided_with_outer_wire = 0;
    }
}

void UpdateKinematics(int i) {

	Vector2 CollidingCu;
	Vector2 norm;
	
	float vx,vy;
	float mag;
		
	int ex = (int) e[i].x;
	int ey = (int) e[i].y;

	CheckCollisionWithWire(ex,ey);
	
	CollidingCu = FindCollidingCu(ex,ey);

	if (collided_with_cu == 1 || collided_with_inner_wire ==1 || collided_with_outer_wire == 1) {
	
		if (collided_with_cu) {
		
				norm.x = ex - CollidingCu.x;
				norm.y = ey - CollidingCu.y;
				
				mag = fabs(sqrt(norm.x*norm.x + norm.y*norm.y));
				
				norm.x /= mag;
				norm.y /= mag;
				
		}
		
		if (collided_with_inner_wire) {

			if ((ex>OFFSET)&&(ex<OFFSET + THICCNESS)) {
			
				// e is in left
				norm.y = 0;
				norm.x = -1;
				
			}

			else if ((ex>1920 - OFFSET - THICCNESS)&&(ex<1920-OFFSET)) {
				// e is in right

				norm.y = 0;
				norm.x = 1;
			}

			else if ((ey>OFFSET)&&(ey<OFFSET + THICCNESS)) {
				// e in top
				norm.x = 0;
				norm.y = -1;
			}

			else {
				// e in bottom
				norm.x = 0;
				norm.y = 1;
			}
			
		}

		else if (collided_with_outer_wire) {
		
				if ((ex>OFFSET)&&(ex<OFFSET + THICCNESS)) {
					// e is in left
					norm.y = 0;
					norm.x = 1;
						
				}
		
				else if ((ex>1920 - OFFSET - THICCNESS)&&(ex<1920-OFFSET)) {
					// e is in right
		
					norm.y = 0;
					norm.x = -1;
				}
		
				else if ((ey>OFFSET)&&(ey<OFFSET + THICCNESS)) {
					// e in top
					norm.x = 0;
					norm.y = -1;
				}
		
				else {
					// e in bottom
					norm.x = 0;
					norm.y = 1;
				}
			}
			
		mag = fabs(sqrt(norm.x*norm.x + norm.y*norm.y));

		if (mag==0) {
			e[i].x += 1;
			norm.x = (float)(ex - CollidingCu.x);
			norm.y = (float)(ey - CollidingCu.y);

			mag = fabs(sqrt(pow(norm.x,2) + pow(norm.y,2)));

			norm.x /= mag;
			norm.y /= mag;
		}
		
		vx = v[i].x;
		vy = v[i].y;
	
		v[i].x = vx - 2*(vx*norm.x + vy*norm.y)*norm.x;
		v[i].y = vy - 2*(vx*norm.x + vy*norm.y)*norm.y;
		
	}	

	if (ex>0 && ex < 1920 && ey > 0 && ey <1080) {
	
		a[i].x = E_field*E[(int)e[i].y][(int)e[i].x].x;
		a[i].y = E_field*E[(int)e[i].y][(int)e[i].x].y;
		
		v[i].x += a[i].x*GetFrameTime();
		v[i].y += a[i].y*GetFrameTime();
			
		e[i].x += v[i].x*GetFrameTime();
		e[i].y += v[i].y*GetFrameTime();
	}

	collided_with_cu = 0;
	collided_with_inner_wire = 0; 
	collided_with_outer_wire = 0;		
		
	
}

//draw the copper atoms

void Draw_Cu() {

    for (int i=0;i<TB_ROWS;i++) {

        for (int j=0;j<TB_COLS;j++) {
        
            DrawCircle(top[i][j].x,top[i][j].y,R_CU,GOLD);
        }
    }

    for (int i=0;i<TB_ROWS;i++) {

        for (int j=0;j<TB_COLS;j++) {
        
            DrawCircle(bottom[i][j].x,bottom[i][j].y,R_CU,GOLD);
        }
    }

    for (int i=0;i<LR_ROWS;i++) {

        for (int j=0;j<LR_COLS;j++) {
        
            DrawCircle(left[i][j].x,left[i][j].y,R_CU,GOLD);
        }
    }

    for (int i=0;i<LR_ROWS;i++) {

        for (int j=0;j<LR_COLS;j++) {
            DrawCircle(right[i][j].x,right[i][j].y,R_CU,GOLD);
        }
    }
}

// now the electrons
void Draw_e() {

	for (int i=0;i<N_E;i++) {
	
		DrawCircle(e[i].x,e[i].y,R_E,BLUE);
	}
}

int main(void) {
    
    Rectangle battery = {OFFSET + THICCNESS + INNER_WIDTH/2 - 120,OFFSET+THICCNESS+INNER_HEIGHT-50,340,200};
    Vector2 textpos = {OFFSET + THICCNESS + INNER_WIDTH/2 - 76, OFFSET+THICCNESS+INNER_HEIGHT+30};
    
    InitWindow(1920,1080,"How DC Circuits actually work");

	Compute_E();
    Init_Cu();
	Init_e();
	
    while(!WindowShouldClose()) {

    	for (int i=0;i<N_E;i++) {
    		UpdateKinematics(i);	
		}
		
        BeginDrawing();
        ClearBackground(BLACK);

        DrawRectangleLines(OFFSET, OFFSET, OUTER_WIDTH, OUTER_HEIGHT, WHITE);
        DrawRectangleLines(OFFSET+THICCNESS, OFFSET+THICCNESS, INNER_WIDTH, INNER_HEIGHT, WHITE);

        Draw_Cu(); // Draw copper atoms
        Draw_e(); // draw electrons
        
        DrawRectangleRec(battery,WHITE); //draw battery
		    DrawText("battery",textpos.x, textpos.y, 65.0f,BLACK);
        EndDrawing();
 
    }
    
    CloseWindow();

    return 0;
}
