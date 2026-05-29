#include "inl.h"
#include "agua.h"
#include "estructuras.h"
#include <stdio.h>

extern cpSpace * modChipmunk_cpEspacio;
extern DataPointer modChipmunk_ListaCuerpos;

#if (defined(_WIN64) || defined(SORR_HOST_POINTER_TABLES))
extern void * portable_x64_sysproc_pointer_param( int * cell );
#define MODCHIPMUNK_POINTER_PARAM(cell,type) (( type * )portable_x64_sysproc_pointer_param( cell ))
#else
#define MODCHIPMUNK_POINTER_PARAM(cell,type) (( type * )( *( cell ) ))
#endif

#define MODCHIPMUNK_WATER_MAX_IDS 1024

#ifdef SORR_IOS_D3_FIRST_RENDER
extern void sorr_ios_d3_note_instance_lookup( int requested_id, const INSTANCE * candidate, const char * reason );
extern char sorr_ios_d3_last_effect_water_event[];
#define MODCHIPMUNK_NOTE_LOOKUP(id,inst,reason) sorr_ios_d3_note_instance_lookup( id, inst, reason )
#define MODCHIPMUNK_NOTE_WATER(...) snprintf( sorr_ios_d3_last_effect_water_event, 1024, __VA_ARGS__ )
#else
#define MODCHIPMUNK_NOTE_LOOKUP(id,inst,reason) do { ( void )( id ); ( void )( inst ); } while ( 0 )
#define MODCHIPMUNK_NOTE_WATER(...) do { } while ( 0 )
#endif

static cpBody * modChipmunkBodyFromInstance( INSTANCE * instance )
{
    DataPointer node;
    int id;

    if ( !instance )
        return NULL;

    id = LOCDWORD( mod_chipmunk, instance, LOC_ID );
    for ( node = modChipmunk_ListaCuerpos ? modChipmunk_ListaCuerpos->sig : NULL;
          node;
          node = node->sig )
    {
        if ( node->father == id && node->body )
            return node->body;
    }

    return NULL;
}

//typedef void (*cpSpaceBBQueryFunc)(cpShape *shape, void *data)
//
//void cpSpaceBBQuery(
//	cpSpace *space, cpBB bb,
//	cpLayers layers, cpGroup group,
//	cpSpaceBBQueryFunc func, void *data
//)

//float trr,x0x,y0x,ts;
//int res;
//
//
//void calcV(cpShape *shape, void *datos){
//cpBody * cuerpo=shape->body;
//float v=0.0;
//    if ( cuerpo!=modChipmunk_cpEspacio->staticBody && ((DataPointer)cuerpo->data)->grupoEfector==1
//        ){
//        INSTANCE * i;
//        float a,b;
//        i=instance_get(((DataPointer)shape->body->data)->father);
//        a=LOCINT32(mod_chipmunk, i, LOC_X)-x0x;
//        b=LOCINT32(mod_chipmunk, i, LOC_Y)-y0x;
//        v+=ts/(float)((float)a*(float)a +(float)b*(float)b+0.0001);
//
//    }
//    res=v>trr;
//}

static inline float Max(float a,float b){
return a>b? a:b;
}

static inline float Min(float a,float b){
return a<b? a:b;
}

static inline int Metaball(int x,int y, int*xp,int*yp, int tam, float t, float tr)
{
    //printf("%d %d %d %p %d\n",params[0],params[1],params[2],params[3],params[4]); fflush(stdout);
    float v=0.0,a,b;
    int z;
    //INSTANCE * i;
    for (z=0;z<tam;z++){
//       i=instance_get(ids[z]);
//       a=LOCINT32(mod_chipmunk, i, LOC_X)-x;
//       b=LOCINT32(mod_chipmunk, i, LOC_Y)-y;
       a=xp[z]-x;
       b=yp[z]-y;
       v+=t/(float)((float)a*(float)a +(float)b*(float)b+0.0001);
    }
    //printf("%f\n",v); fflush(stdout);
    return v>tr;

}
//encontrar minx y miny para el pintado...
int modChipmunkPintaAgua(INSTANCE * my, int * params){
    GRAPH * map=bitmap_get( params[0], params[1] );
    int anc;
    int alt;
    int x4,y4;
    int *ids=MODCHIPMUNK_POINTER_PARAM( &params[3], int );
    int tam=params[4];
    int x[MODCHIPMUNK_WATER_MAX_IDS];
    int y[MODCHIPMUNK_WATER_MAX_IDS];
    int minx, miny, maxx=0,maxy=0;
    INSTANCE * i;
    int input_tam=tam;
    int valid=0;

    int z;

    ( void )my;

    if ( !map || !ids || tam <= 0 || tam > MODCHIPMUNK_WATER_MAX_IDS )
    {
        MODCHIPMUNK_NOTE_WATER( "chipmunk_draw_water_invalid map=%p ids=%p tam=%d file=%d graph=%d",
                                ( void * )map, ( void * )ids, tam, params[0], params[1] );
        return 0;
    }

    anc=map->width;
    alt=map->height;
    minx=anc;
    miny=alt;

    for (z=0;z<tam;z++){
       i=instance_get(ids[z]);
       if ( !i )
       {
          MODCHIPMUNK_NOTE_LOOKUP( ids[z], i, "chipmunk-draw-water-missing-id" );
          continue;
       }
       x[valid]=x4=LOCINT32(mod_chipmunk, i, LOC_X)-params[8];
       y[valid]=y4=LOCINT32(mod_chipmunk, i, LOC_Y)-params[9];
       //printf("%d %d\n",params[8],params[9]);
       if (minx>x4)
        minx=x4;
       else if (maxx<x4)
        maxx=x4;

       if (miny>y4)
        miny=y4;
       else if (maxy<y4)
        maxy=y4;

       valid++;
    }

    tam=valid;
    if ( tam <= 0 )
    {
        MODCHIPMUNK_NOTE_WATER( "chipmunk_draw_water_no_valid_ids ids=%p input_tam=%d", ( void * )ids, input_tam );
        return 0;
    }

    if (tam==1){
        maxx=minx;
        maxy=miny;
    }

    int reaj=params[7];
    minx=Max(minx-reaj,0);
    miny=Max(miny-reaj,0);
    maxx=Min(maxx+reaj,anc);
    maxy=Min(maxy+reaj,alt);

    MODCHIPMUNK_NOTE_WATER( "chipmunk_draw_water ok ids=%p input_tam=%d valid=%d file=%d graph=%d map=%dx%d bounds=%d,%d-%d,%d offset=%d,%d",
                            ( void * )ids, input_tam, tam, params[0], params[1], anc, alt, minx, miny, maxx, maxy, params[8], params[9] );

    //int color=params[2];
    for (x4=minx;x4<maxx;x4++){
        for (y4=miny;y4<maxy;y4++){
             if ( Metaball(x4,y4,x,y,tam,*(float*)&params[5],*(float*)&params[6]))
            // *( uint32_t * ) (( uint8_t * ) map->data + map->pitch * y4 + ( x4 << 2 ))=color;
                gr_put_pixel(map,x4,y4,params[2]);
        }
    }
    return 1;
}


int modChipmunkMetaball(INSTANCE * my, int * params)
{
    //printf("%d %d %d %p %d\n",params[0],params[1],params[2],params[3],params[4]); fflush(stdout);
    float v=0.0,a,b,t;
    int z;
    int *ids=MODCHIPMUNK_POINTER_PARAM( &params[3], int );
    INSTANCE * i;
    int valid=0;

    ( void )my;

    if ( !ids || params[4] <= 0 || params[4] > MODCHIPMUNK_WATER_MAX_IDS )
    {
        MODCHIPMUNK_NOTE_WATER( "chipmunk_metaball_invalid ids=%p tam=%d point=%d,%d",
                                ( void * )ids, params[4], params[0], params[1] );
        return 0;
    }

    t=(float)params[2];
    for (z=0;z<params[4];z++){
       i=instance_get(ids[z]);
       if ( !i )
       {
          MODCHIPMUNK_NOTE_LOOKUP( ids[z], i, "chipmunk-metaball-missing-id" );
          continue;
       }
       a=LOCINT32(mod_chipmunk, i, LOC_X)-params[0];
       b=LOCINT32(mod_chipmunk, i, LOC_Y)-params[1];
       v+=t/(float)((float)a*(float)a +(float)b*(float)b+0.0001);
       valid++;
    }
    //printf("%f\n",v); fflush(stdout);
    MODCHIPMUNK_NOTE_WATER( "chipmunk_metaball ids=%p tam=%d valid=%d point=%d,%d value=%f threshold=%f",
                            ( void * )ids, params[4], valid, params[0], params[1], v, *(float*)&params[5] );
    return valid > 0 && v>(*(float*)&params[5]);
}

int modChipmunkSetEfector(INSTANCE * my, int * params){
    cpShape * shape=MODCHIPMUNK_POINTER_PARAM( &params[0], cpShape );

    ( void )my;

    if ( !shape || !shape->body || !shape->body->data )
    {
        MODCHIPMUNK_NOTE_WATER( "chipmunk_set_efector_invalid shape=%p body=%p data=%p value=%d",
                                ( void * )shape,
                                shape ? ( void * )shape->body : NULL,
                                ( shape && shape->body ) ? ( void * )shape->body->data : NULL,
                                params[1] );
        return 0;
    }

    ((DataPointer)shape->body->data)->grupoEfector=params[1];
    MODCHIPMUNK_NOTE_WATER( "chipmunk_set_efector ok shape=%p body=%p father=%d value=%d",
                            ( void * )shape,
                            ( void * )shape->body,
                            ((DataPointer)shape->body->data)->father,
                            params[1] );
    return 1;
}

//typedef struct WaterSs{
//  int*ids;
//  int tam;
//  float kNorm;
//  float kNearNorm;
//  float kRestDensity;
//  float kStiffness;
//  float kNearStiffness;
//  float kSurfaceTension;
//  float kLinearViscocity;
//  float kQuadraticViscocity;
//  float KH;
//}WaterS;

int modChipmunkAgua(int*ids,int tam, float kNorm, float kNearNorm,float kRestDensity,
                    float kStiffness,float kNearStiffness, float kDT, float kSurfaceTension,
                    float kLinearViscocity,float kQuadraticViscocity, float KH){
    int x[MODCHIPMUNK_WATER_MAX_IDS];
    int y[MODCHIPMUNK_WATER_MAX_IDS];
    float vx[MODCHIPMUNK_WATER_MAX_IDS];
    float vy[MODCHIPMUNK_WATER_MAX_IDS];
    INSTANCE *is;//,*in[tam];
    cpBody* bd[MODCHIPMUNK_WATER_MAX_IDS],*bdy;
    float mass=1;
    float densidad[MODCHIPMUNK_WATER_MAX_IDS];
    float nearDensidad[MODCHIPMUNK_WATER_MAX_IDS];
    float p[MODCHIPMUNK_WATER_MAX_IDS];
    float nearP[MODCHIPMUNK_WATER_MAX_IDS];
    int i,j;
    float kDT2=kDT*kDT;
    int input_tam=tam;
    int valid=0;

    if ( !ids || tam <= 0 || tam > MODCHIPMUNK_WATER_MAX_IDS )
    {
        MODCHIPMUNK_NOTE_WATER( "chipmunk_emulate_water_invalid ids=%p tam=%d kdt=%f", ( void * )ids, tam, kDT );
        return 0;
    }

    for (i=0;i<tam;i++){
       is=instance_get(ids[i]);
       if ( !is )
       {
          MODCHIPMUNK_NOTE_LOOKUP( ids[i], is, "chipmunk-emulate-water-missing-id" );
          continue;
       }
       //printf("%d\n",ids[i]);
       bdy=modChipmunkBodyFromInstance( is );
       if ( !bdy )
       {
          MODCHIPMUNK_NOTE_LOOKUP( ids[i], is, "chipmunk-emulate-water-missing-body" );
          continue;
       }
       x[valid]=LOCINT32(mod_chipmunk, is, LOC_X);
       y[valid]=LOCINT32(mod_chipmunk, is, LOC_Y);
       bd[valid]=bdy;
       vx[valid]=bdy->v.x;
       vy[valid]=bdy->v.y;
       valid++;

    }

    tam=valid;
    if ( tam <= 0 )
    {
        MODCHIPMUNK_NOTE_WATER( "chipmunk_emulate_water_no_valid_ids ids=%p input_tam=%d", ( void * )ids, input_tam );
        return 0;
    }

    MODCHIPMUNK_NOTE_WATER( "chipmunk_emulate_water begin ids=%p input_tam=%d valid=%d kdt=%f kh=%f",
                            ( void * )ids, input_tam, tam, kDT, KH );

    float KH2=KH*KH;

    for (i=0;i<tam;i++){
        float density=0,nearDensity=0;
        for (j=0;j<tam;j++){
            if (i==j)
                continue;
            float dx,dy,r2,a,r;
            dx=x[j]-x[i];
            dy=y[j]-y[i];
            r2=dx*dx+dy*dy;
            if (r2>KH2)//no se...
                continue;
            r=sqrt(r2);
            a=1-r/KH;
            float a3m=a*a*a*mass;
            density+=a3m*kNorm;
            nearDensity+=a3m*a*kNearNorm;
        }
        densidad[i]=density;
        nearDensidad[i]=nearDensity;
        p[i]=kStiffness*(density-mass*kRestDensity);
        nearP[i]=kNearStiffness*nearDensity;
    }

    for (i=0;i<tam;i++){
        float x0=0,y0=0;
        for (j=0;j<tam;j++){
            if (i==j)
                continue;
            float dx,dy,r2,a,d,r;
            dx=x[j]-x[i];
            dy=y[j]-y[i];
            r2=dx*dx+dy*dy;
            if (r2>KH2)//no se...
                continue;
            r=sqrt(r2);
            a=1-r/KH;
            float a2=a*a;
            d=kDT2*((nearP[i]+nearP[j])* a2*a*kNearNorm +(p[i]+p[j])*a2*kNorm)/2;
            float rmd=d/(r*mass);
            x0-=rmd*dx;
            y0-=rmd*dy;

            //superficie
            rmd=kSurfaceTension*a2*kNorm;
            x0+=rmd*dx;
            y0+=rmd*dy;

            //viscocidad
            float du=vx[i]-vx[j];
            float dv=vy[i]-vy[j];
            float u=du*dx+dv*dy;

            if(u>0){
                u/=r;
                float It=0.5*kDT*a*(kLinearViscocity*u+kQuadraticViscocity*u*u)*kDT;
                x0-=It*dx;
                y0-=It*dy;
            }

        }
        cpBodySetVel(bd[i],cpv(vx[i]+(x0*kDT),vy[i]+(y0*kDT)) );
        cpBodyUpdateVelocity(bd[i],cpv(*(float *)GLOADDR(mod_chipmunk,GLO_GRAVITY_X),*(float *)GLOADDR(mod_chipmunk,GLO_GRAVITY_Y)), *(float *)GLOADDR(mod_chipmunk,GLO_DAMPING),kDT);
//cpSpaceReindexShapesForBody(modChipmunk_cpEspacio,bd[i]);
        //LOCINT32(mod_chipmunk, in[i], LOC_X)+=x0;
       // LOCINT32(mod_chipmunk, in[i], LOC_Y)+=y0;

    }
    return 1;
}

int modChipmunkEmulateAgua(INSTANCE * my, int * params){
    float kdt=*(float *)GLOADDR(mod_chipmunk,GLO_INTERVAL)/(float)GLODWORD(mod_chipmunk,GLO_PHRESOLUTION);
    ( void )my;
#if (defined(_WIN64) || defined(SORR_HOST_POINTER_TABLES))
    int *ws_cells=MODCHIPMUNK_POINTER_PARAM( &params[0], int );
    int *ids=NULL;
    int size=0;

    if ( ws_cells )
    {
        ids=MODCHIPMUNK_POINTER_PARAM( &ws_cells[0], int );
        size=ws_cells[1];
    }

    if ( !ws_cells || !ids || size <= 0 || size > MODCHIPMUNK_WATER_MAX_IDS )
    {
        MODCHIPMUNK_NOTE_WATER( "chipmunk_emulate_water_struct_invalid ws_cells=%p ids=%p size=%d kdt=%f",
                                ( void * )ws_cells,
                                ( void * )ids,
                                size,
                                kdt );
        return 0 ;
    }

    MODCHIPMUNK_NOTE_WATER( "chipmunk_emulate_water_struct ws_cells=%p ids=%p size=%d kdt=%f",
                            ( void * )ws_cells, ( void * )ids, size, kdt );
    modChipmunkAgua(ids,size,*(float*)&ws_cells[2],*(float*)&ws_cells[3],*(float*)&ws_cells[4],
                    *(float*)&ws_cells[5],*(float*)&ws_cells[6],kdt,*(float*)&ws_cells[7],
                    *(float*)&ws_cells[8],*(float*)&ws_cells[9],*(float*)&ws_cells[10]);
#else
    WaterS *ws=MODCHIPMUNK_POINTER_PARAM( &params[0], WaterS );
    //printf("%f\n",kdt);fflush(stdout);
    if ( !ws || ws->ids==0 || ws->size <= 0 || ws->size > MODCHIPMUNK_WATER_MAX_IDS )
    {
        MODCHIPMUNK_NOTE_WATER( "chipmunk_emulate_water_struct_invalid ws=%p ids=%p size=%d kdt=%f",
                                ( void * )ws,
                                ws ? ( void * )ws->ids : NULL,
                                ws ? ws->size : 0,
                                kdt );
        return 0 ;
    }
    MODCHIPMUNK_NOTE_WATER( "chipmunk_emulate_water_struct ws=%p ids=%p size=%d kdt=%f",
                            ( void * )ws, ( void * )ws->ids, ws->size, kdt );
    modChipmunkAgua(ws->ids,ws->size,ws->kNorm,ws->kNearNorm,ws->kRestDensity,
                    ws->kStiffness,ws->kNearStiffness,kdt,ws->kSurfaceTension,
                    ws->kLinearViscocity,ws->kQuadraticViscocity,ws->KH);
#endif
    return 1;
}
