#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

/* ── Dimensions ─────────────────────────────────────────────────────────── */
#define W   1100
#define H   740
#define NAV_H  130   /* hauteur totale header+nav */

/* ── Limites ─────────────────────────────────────────────────────────────── */
#define MAX_COMPTES   100
#define MAX_HISTORY   20   /* transactions par compte */
#define CSV_FILE      "Data.csv"

/* ── Écrans ──────────────────────────────────────────────────────────────── */
#define ECRAN_ACCUEIL   0
#define ECRAN_LISTE     1
#define ECRAN_CREER     2
#define ECRAN_LOGIN     3
#define ECRAN_DEPOT     4
#define ECRAN_RETRAIT   5
#define ECRAN_VIREMENT  6
#define ECRAN_MONCOMPTE 7

/* ── Palette luxe ────────────────────────────────────────────────────────── */
#define COL_BG        (SDL_Color){ 8,  10,  18, 255}
#define COL_BG2       (SDL_Color){12,  15,  26, 255}
#define COL_HEADER    (SDL_Color){14,  17,  32, 255}
#define COL_CARD      (SDL_Color){22,  26,  48, 255}
#define COL_CARD2     (SDL_Color){30,  34,  62, 255}
#define COL_CARD3     (SDL_Color){40,  45,  80, 255}
#define COL_CARD4     (SDL_Color){50,  56,  98, 255}
#define COL_BORDER    (SDL_Color){60,  66, 110, 255}
#define COL_BORDER2   (SDL_Color){90,  98, 160, 255}
/* Or / accent principal */
#define COL_GOLD      (SDL_Color){230, 170,  30, 255}
#define COL_GOLD2     (SDL_Color){255, 210,  80, 255}
#define COL_GOLD3     (SDL_Color){180, 130,  10, 255}
#define COL_GOLD_DIM  (SDL_Color){100,  76,  10, 255}
/* Vert succès */
#define COL_GREEN     (SDL_Color){ 20, 210, 150, 255}
#define COL_GREEN2    (SDL_Color){  0, 160, 110, 255}
#define COL_GREEN_DIM (SDL_Color){  6,  60,  42, 255}
/* Rouge danger */
#define COL_RED       (SDL_Color){240,  70,  70, 255}
#define COL_RED2      (SDL_Color){190,  45,  45, 255}
#define COL_RED_DIM   (SDL_Color){ 60,  14,  14, 255}
/* Bleu action */
#define COL_BLUE      (SDL_Color){100, 110, 255, 255}
#define COL_BLUE2     (SDL_Color){ 70,  80, 220, 255}
#define COL_BLUE_DIM  (SDL_Color){ 22,  25,  70, 255}
/* Violet premium */
#define COL_PURPLE    (SDL_Color){170,  90, 255, 255}
#define COL_PURPLE2   (SDL_Color){130,  60, 210, 255}
/* Textes */
#define COL_TEXT      (SDL_Color){230, 232, 240, 255}
#define COL_TEXT2     (SDL_Color){110, 118, 148, 255}
#define COL_TEXT3     (SDL_Color){ 70,  78, 110, 255}
#define COL_WHITE     (SDL_Color){255, 255, 255, 255}
#define COL_BLACK     (SDL_Color){  0,   0,   0, 255}
/* Session */
#define COL_SESSION   (SDL_Color){ 10,  24,  18, 255}
#define COL_LOGOUT    (SDL_Color){ 48,  14,  14, 255}

/* ── Historique de transactions ─────────────────────────────────────────── */
typedef struct {
    char  type[16];   /* "DEPOT", "RETRAIT", "VIREMENT+" , "VIREMENT-" */
    float montant;
    float solde_apres;
    char  detail[64]; /* ex: vers 012345678901 */
    char  date[20];
} Transaction;

/* ── Compte ──────────────────────────────────────────────────────────────── */
typedef struct {
    char        numero[13];
    char        code_acces[5];
    float       solde;
    char        nom[64];
    char        prenom[64];
    char        naissance[16];
    char        email[128];
    Transaction history[MAX_HISTORY];
    int         nb_transactions;
} Compte;

/* ── Particule décorative ────────────────────────────────────────────────── */
typedef struct {
    float x, y, vx, vy;
    float alpha;
    int   size;
    int   type; /* 0=cercle, 1=losange */
} Particle;

#define MAX_PARTICLES 40

static Compte   comptes[MAX_COMPTES];
static int      totalComptes        = 0;
static char     compte_connecte[13] = "";
static int      login_ok            = 0;
static int      rand_seeded         = 0;
static Particle particles[MAX_PARTICLES];
static int      particles_init      = 0;

/* ── Flash message ───────────────────────────────────────────────────────── */
static char   msg_text[256] = "";
static int    msg_ok        = 1;
static Uint32 msg_timer     = 0;
static int    msg_icon      = 0; /* 0=check, 1=x, 2=info */

/* ────────────────────────────────────────────────────────────────────────── */
/*                         LOGIQUE BANCAIRE                                  */
/* ────────────────────────────────────────────────────────────────────────── */

void generer_numero(char *out) {
    if (!rand_seeded) { srand((unsigned)time(NULL)); rand_seeded = 1; }
    out[0] = '1' + rand() % 9;
    for (int i = 1; i < 12; i++) out[i] = '0' + rand() % 10;
    out[12] = '\0';
}
void generer_code(char *out) {
    for (int i = 0; i < 4; i++) out[i] = '0' + rand() % 10;
    out[4] = '\0';
}
int numero_existe(const char *num) {
    for (int i = 0; i < totalComptes; i++)
        if (strcmp(comptes[i].numero, num) == 0) return 1;
    return 0;
}

/* Ajouter une transaction à l'historique */
void add_transaction(Compte *c, const char *type, float montant,
                     float solde_apres, const char *detail) {
    /* Décaler si plein */
    if (c->nb_transactions >= MAX_HISTORY) {
        for (int i = 0; i < MAX_HISTORY - 1; i++)
            c->history[i] = c->history[i + 1];
        c->nb_transactions = MAX_HISTORY - 1;
    }
    Transaction *t = &c->history[c->nb_transactions++];
    strncpy(t->type, type, 15);
    t->montant    = montant;
    t->solde_apres= solde_apres;
    strncpy(t->detail, detail ? detail : "", 63);

    /* Date actuelle */
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    strftime(t->date, sizeof(t->date), "%d/%m/%Y %H:%M", tm_info);
}

void sauvegarder_csv(void) {
    FILE *f = fopen(CSV_FILE, "w");
    if (!f) return;
    fprintf(f, "Numero,CodeAcces,Solde,Nom,Prenom,DateNaissance,Email\n");
    for (int i = 0; i < totalComptes; i++)
        fprintf(f, "%s,%s,%.2f,%s,%s,%s,%s\n",
            comptes[i].numero, comptes[i].code_acces, comptes[i].solde,
            comptes[i].nom, comptes[i].prenom,
            comptes[i].naissance, comptes[i].email);
    fclose(f);
}
void charger_csv(void) {
    FILE *f = fopen(CSV_FILE, "r");
    if (!f) return;
    char line[512];
    fgets(line, sizeof(line), f);
    while (fgets(line, sizeof(line), f) && totalComptes < MAX_COMPTES) {
        Compte *c = &comptes[totalComptes];
        char *tok = strtok(line, ",");
        if (!tok) continue;
        strncpy(c->numero, tok, 12); c->numero[12] = '\0';
        tok = strtok(NULL, ","); if (!tok) continue;
        strncpy(c->code_acces, tok, 4); c->code_acces[4] = '\0';
        tok = strtok(NULL, ","); if (!tok) continue;
        c->solde = (float)atof(tok);
        tok = strtok(NULL, ","); if (!tok) continue;
        strncpy(c->nom, tok, 63);
        tok = strtok(NULL, ","); if (!tok) continue;
        strncpy(c->prenom, tok, 63);
        tok = strtok(NULL, ","); if (!tok) continue;
        strncpy(c->naissance, tok, 15);
        tok = strtok(NULL, "\n"); if (!tok) continue;
        strncpy(c->email, tok, 127);
        c->nom[strcspn(c->nom,"\r\n")]            = '\0';
        c->prenom[strcspn(c->prenom,"\r\n")]      = '\0';
        c->naissance[strcspn(c->naissance,"\r\n")]= '\0';
        c->email[strcspn(c->email,"\r\n")]         = '\0';
        c->nb_transactions = 0;
        totalComptes++;
    }
    fclose(f);
}
const char* creerCompte(const char *nom, const char *prenom,
                         const char *naissance, const char *email) {
    if (totalComptes >= MAX_COMPTES) return NULL;
    Compte *c = &comptes[totalComptes];
    do { generer_numero(c->numero); } while (numero_existe(c->numero));
    generer_code(c->code_acces);
    c->solde           = 0.0f;
    c->nb_transactions = 0;
    strncpy(c->nom,       nom,       63);
    strncpy(c->prenom,    prenom,    63);
    strncpy(c->naissance, naissance, 15);
    strncpy(c->email,     email,     127);
    totalComptes++;
    sauvegarder_csv();
    return comptes[totalComptes-1].numero;
}
Compte* chercherCompte(const char *numero) {
    for (int i = 0; i < totalComptes; i++)
        if (strcmp(comptes[i].numero, numero) == 0) return &comptes[i];
    return NULL;
}
int verifierLogin(const char *numero, const char *code) {
    Compte *c = chercherCompte(numero);
    if (!c) return 0;
    if (strcmp(c->code_acces, code) != 0) return 0;
    strncpy(compte_connecte, c->numero, 12);
    compte_connecte[12] = '\0';
    login_ok = 1;
    return 1;
}
void deconnecter(void) {
    login_ok = 0;
    compte_connecte[0] = '\0';
}

int depot(float montant) {
    if (!login_ok) return -4;
    Compte *c = chercherCompte(compte_connecte);
    if (!c) return -1;
    if (montant <= 0) return -2;
    c->solde += montant;
    add_transaction(c, "DEPOT", montant, c->solde, "");
    sauvegarder_csv();
    return 1;
}
int retrait(float montant) {
    if (!login_ok) return -4;
    Compte *c = chercherCompte(compte_connecte);
    if (!c) return -1;
    if (montant <= 0) return -2;
    if (c->solde < montant) return -3;
    c->solde -= montant;
    add_transaction(c, "RETRAIT", montant, c->solde, "");
    sauvegarder_csv();
    return 1;
}
int virement(const char *destinataire, float montant) {
    if (!login_ok) return -4;
    Compte *source = chercherCompte(compte_connecte);
    Compte *dest   = chercherCompte(destinataire);
    if (!source || !dest) return -1;
    if (montant <= 0) return -2;
    if (source->solde < montant) return -3;
    if (strcmp(source->numero, destinataire) == 0) return -5;
    source->solde -= montant;
    dest->solde   += montant;
    char detail_src[64], detail_dst[64];
    snprintf(detail_src, 64, "Vers %s", destinataire);
    snprintf(detail_dst, 64, "De %s",   source->numero);
    add_transaction(source, "VIREMENT-", montant, source->solde, detail_src);
    add_transaction(dest,   "VIREMENT+", montant, dest->solde,   detail_dst);
    sauvegarder_csv();
    return 1;
}
int supprimerCompte(void) {
    if (!login_ok) return -4;
    for (int i = 0; i < totalComptes; i++) {
        if (strcmp(comptes[i].numero, compte_connecte) == 0) {
            for (int j = i; j < totalComptes-1; j++) comptes[j] = comptes[j+1];
            totalComptes--;
            sauvegarder_csv();
            deconnecter();
            return 1;
        }
    }
    return -1;
}

/* ────────────────────────────────────────────────────────────────────────── */
/*                         PRIMITIVES DE RENDU                               */
/* ────────────────────────────────────────────────────────────────────────── */

void set_message(const char *txt, int ok) {
    strncpy(msg_text, txt, 255);
    msg_ok    = ok;
    msg_icon  = ok ? 0 : 1;
    msg_timer = SDL_GetTicks();
}

/* Dessine du texte simple */
void draw_text(SDL_Renderer *r, TTF_Font *font, const char *text,
               int x, int y, SDL_Color color) {
    if (!text || !*text) return;
    SDL_Surface *s = TTF_RenderUTF8_Blended(font, text, color);
    if (!s) return;
    SDL_Texture *t = SDL_CreateTextureFromSurface(r, s);
    SDL_Rect rect  = {x, y, s->w, s->h};
    SDL_RenderCopy(r, t, NULL, &rect);
    SDL_FreeSurface(s);
    SDL_DestroyTexture(t);
}

/* Texte centré horizontalement */
void draw_text_centered(SDL_Renderer *r, TTF_Font *font, const char *text,
                        int cx, int y, SDL_Color color) {
    if (!text || !*text) return;
    int w; TTF_SizeUTF8(font, text, &w, NULL);
    draw_text(r, font, text, cx - w/2, y, color);
}

/* Rectangle plein */
void fill_rect(SDL_Renderer *r, SDL_Rect rect, SDL_Color c) {
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
    SDL_RenderFillRect(r, &rect);
}

/* Rectangle arrondi rempli (pixel par pixel pour les coins) */
void draw_rounded_rect(SDL_Renderer *r, SDL_Rect rect, int radius, SDL_Color c) {
    if (radius <= 0) { fill_rect(r, rect, c); return; }
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
    SDL_Rect cr = {rect.x + radius, rect.y,          rect.w - 2*radius, rect.h};
    SDL_Rect lr = {rect.x,          rect.y + radius,  radius,            rect.h - 2*radius};
    SDL_Rect rr = {rect.x+rect.w-radius, rect.y+radius, radius,          rect.h - 2*radius};
    SDL_RenderFillRect(r, &cr);
    SDL_RenderFillRect(r, &lr);
    SDL_RenderFillRect(r, &rr);
    for (int wi = 0; wi < radius; wi++) {
        for (int hi = 0; hi < radius; hi++) {
            int dx = radius - wi, dy = radius - hi;
            if (dx*dx + dy*dy <= radius*radius) {
                SDL_RenderDrawPoint(r, rect.x+wi,             rect.y+hi);
                SDL_RenderDrawPoint(r, rect.x+rect.w-wi-1,    rect.y+hi);
                SDL_RenderDrawPoint(r, rect.x+wi,             rect.y+rect.h-hi-1);
                SDL_RenderDrawPoint(r, rect.x+rect.w-wi-1,    rect.y+rect.h-hi-1);
            }
        }
    }
}

/* Bordure arrondie */
void draw_rounded_border(SDL_Renderer *r, SDL_Rect rect, int radius,
                         SDL_Color c, int thickness) {
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
    for (int t = 0; t < thickness; t++) {
        SDL_Rect inner = {rect.x+t, rect.y+t, rect.w-2*t, rect.h-2*t};
        SDL_RenderDrawRect(r, &inner);
    }
}

/* Cercle rempli */
void draw_filled_circle(SDL_Renderer *r, int cx, int cy, int radius, SDL_Color c) {
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
    for (int dx = -radius; dx <= radius; dx++)
        for (int dy = -radius; dy <= radius; dy++)
            if (dx*dx + dy*dy <= radius*radius)
                SDL_RenderDrawPoint(r, cx+dx, cy+dy);
}

/* Dégradé vertical sur un rectangle */
void draw_gradient_rect(SDL_Renderer *r, SDL_Rect rect,
                        SDL_Color top, SDL_Color bot) {
    for (int i = 0; i < rect.h; i++) {
        float t = (float)i / (float)(rect.h - 1);
        Uint8 rr = (Uint8)(top.r + t*(bot.r - top.r));
        Uint8 gg = (Uint8)(top.g + t*(bot.g - top.g));
        Uint8 bb = (Uint8)(top.b + t*(bot.b - top.b));
        SDL_SetRenderDrawColor(r, rr, gg, bb, 255);
        SDL_RenderDrawLine(r, rect.x, rect.y+i, rect.x+rect.w-1, rect.y+i);
    }
}

/* Bouton avec dégradé + bordure lumineuse + texte */
void draw_button_gradient(SDL_Renderer *r, TTF_Font *font, const char *label,
                          SDL_Rect btn, SDL_Color top, SDL_Color bot,
                          SDL_Color border, SDL_Color tc) {
    draw_gradient_rect(r, btn, top, bot);
    /* Coins arrondis par-dessus : on efface avec la couleur de fond */
    /* (approche simplifiée : on dessine les coins en couleur de fond) */
    draw_rounded_border(r, btn, 10, border, 1);
    int tw, th; TTF_SizeUTF8(font, label, &tw, &th);
    draw_text(r, font, label, btn.x+(btn.w-tw)/2, btn.y+(btn.h-th)/2, tc);
}

/* Bouton simple arrondi */
void draw_button(SDL_Renderer *r, TTF_Font *font, const char *label,
                 SDL_Rect btn, SDL_Color bg, SDL_Color tc) {
    draw_rounded_rect(r, btn, 10, bg);
    int tw, th; TTF_SizeUTF8(font, label, &tw, &th);
    draw_text(r, font, label, btn.x+(btn.w-tw)/2, btn.y+(btn.h-th)/2, tc);
}

/* Champ de saisie amélioré */
void draw_input(SDL_Renderer *r, TTF_Font *flabel, TTF_Font *finput,
                const char *label, const char *value,
                int x, int y, int w, int active, Uint32 ticks, int password) {
    SDL_Color bg_col = active ? (SDL_Color){34, 40, 72, 255} : COL_CARD;
    SDL_Rect  box    = {x, y, w, 54};

    /* Fond + ombre légère */
    SDL_Rect shadow = {x+2, y+2, w, 54};
    SDL_SetRenderDrawColor(r, 0, 0, 0, 40);
    SDL_RenderFillRect(r, &shadow);

    draw_rounded_rect(r, box, 12, bg_col);

    /* Bordure colorée si actif */
    SDL_Color border_col = active ? COL_GOLD : COL_BORDER;
    int thick = active ? 2 : 1;
    SDL_SetRenderDrawColor(r, border_col.r, border_col.g, border_col.b, active ? 220 : 120);
    for (int t = 0; t < thick; t++) {
        SDL_Rect b2 = {box.x+t, box.y+t, box.w-2*t, box.h-2*t};
        SDL_RenderDrawRect(r, &b2);
    }

    /* Barre de soulignage en bas si actif */
    if (active) {
        SDL_SetRenderDrawColor(r, COL_GOLD.r, COL_GOLD.g, COL_GOLD.b, 200);
        SDL_RenderDrawLine(r, x+12, y+52, x+w-12, y+52);
    }

    /* Label flottant */
    SDL_Color lc = active ? COL_GOLD : COL_TEXT2;
    draw_text(r, flabel, label, x+4, y-20, lc);

    /* Affichage */
    char display[256];
    if (password && strlen(value) > 0) {
        int len = (int)strlen(value);
        for (int i = 0; i < len && i < 255; i++) display[i] = '*';
        display[len] = '\0';
    } else {
        strncpy(display, value, 255); display[255] = '\0';
    }
    draw_text(r, finput, display, x+16, y+15, COL_TEXT);

    /* Curseur clignotant */
    if (active && (ticks/500)%2 == 0) {
        int tw = 0;
        if (strlen(display) > 0) TTF_SizeUTF8(finput, display, &tw, NULL);
        SDL_SetRenderDrawColor(r, COL_GOLD.r, COL_GOLD.g, COL_GOLD.b, 255);
        SDL_RenderDrawLine(r, x+16+tw+1, y+11, x+16+tw+1, y+42);
    }
}

/* Séparateur avec dégradé */
void draw_separator(SDL_Renderer *r, int y, int x1, int x2) {
    for (int x = x1; x < x2; x++) {
        float t  = (float)(x - x1) / (x2 - x1);
        float a  = (t < 0.5f) ? t * 2.0f : (1.0f - t) * 2.0f;
        Uint8 alpha = (Uint8)(a * 100);
        SDL_SetRenderDrawColor(r, COL_BORDER.r, COL_BORDER.g, COL_BORDER.b, alpha);
        SDL_RenderDrawPoint(r, x, y);
    }
}

/* Message flash premium */
void draw_message(SDL_Renderer *r, TTF_Font *font, TTF_Font *fsmall, Uint32 ticks) {
    if (!strlen(msg_text)) return;
    Uint32 elapsed = ticks - msg_timer;
    if (elapsed > 4500) { msg_text[0] = '\0'; return; }
    Uint8 alpha = (elapsed > 3700) ? (Uint8)(255*(1.0f-(float)(elapsed-3700)/800.0f)) : 255;

    SDL_Color bg = msg_ok
        ? (SDL_Color){8, 42, 28, alpha}
        : (SDL_Color){50, 10, 10, alpha};
    SDL_Color tc = msg_ok
        ? (SDL_Color){20, 210, 150, alpha}
        : (SDL_Color){240, 70,  70, alpha};

    int mw = W - 80;
    SDL_Rect box  = {40, H - 72, mw, 52};

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 0, 0, 0, alpha/2);
    SDL_Rect shadow = {42, H-70, mw, 52};
    SDL_RenderFillRect(r, &shadow);

    draw_rounded_rect(r, box, 14, bg);
    SDL_SetRenderDrawColor(r, tc.r, tc.g, tc.b, alpha);
    SDL_RenderDrawRect(r, &box);

    /* Icône */
    const char *icon = msg_ok ? " [OK] " : " [!!] ";
    draw_text(r, font, icon, 60, H-58, tc);

    draw_text(r, font, msg_text, 120, H-58, tc);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}

/* Carte stat premium */
void draw_stat_card(SDL_Renderer *r, TTF_Font *flabel, TTF_Font *fval,
                    TTF_Font *ficon,
                    const char *icon, const char *label, const char *val,
                    SDL_Rect rect, SDL_Color accent) {
    /* Ombre */
    SDL_SetRenderDrawColor(r, 0,0,0,60);
    SDL_Rect sh = {rect.x+3, rect.y+3, rect.w, rect.h};
    SDL_RenderFillRect(r, &sh);

    draw_rounded_rect(r, rect, 14, COL_CARD);

    /* Trait supérieur coloré */
    SDL_SetRenderDrawColor(r, accent.r, accent.g, accent.b, 200);
    SDL_Rect top = {rect.x+14, rect.y, rect.w-28, 3};
    SDL_RenderFillRect(r, &top);

    /* Trait latéral */
    SDL_SetRenderDrawColor(r, accent.r, accent.g, accent.b, 80);
    SDL_Rect side = {rect.x, rect.y+14, 3, rect.h-28};
    SDL_RenderFillRect(r, &side);

    /* Cercle icône */
    SDL_SetRenderDrawColor(r, accent.r, accent.g, accent.b, 30);
    SDL_Rect icon_bg = {rect.x+rect.w-56, rect.y+10, 42, 42};
    draw_rounded_rect(r, icon_bg, 8, (SDL_Color){accent.r,accent.g,accent.b,25});
    if (ficon && icon)
        draw_text_centered(r, ficon, icon, rect.x+rect.w-35, rect.y+18, accent);

    draw_text(r, flabel, label, rect.x+14, rect.y+16, COL_TEXT2);
    draw_text(r, fval,   val,   rect.x+14, rect.y+38, accent);
}

/* ── Particules ──────────────────────────────────────────────────────────── */
void init_particles(void) {
    if (particles_init) return;
    if (!rand_seeded) { srand((unsigned)time(NULL)); rand_seeded = 1; }
    for (int i = 0; i < MAX_PARTICLES; i++) {
        particles[i].x     = (float)(rand() % W);
        particles[i].y     = (float)(rand() % H);
        particles[i].vx    = ((float)(rand()%100)/100.0f - 0.5f) * 0.4f;
        particles[i].vy    = ((float)(rand()%100)/100.0f - 0.5f) * 0.3f;
        particles[i].alpha = (float)(rand()%60 + 20);
        particles[i].size  = rand()%3 + 1;
        particles[i].type  = rand()%2;
    }
    particles_init = 1;
}
void update_particles(void) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        particles[i].x += particles[i].vx;
        particles[i].y += particles[i].vy;
        if (particles[i].x < 0)  particles[i].x = W;
        if (particles[i].x > W)  particles[i].x = 0;
        if (particles[i].y < 0)  particles[i].y = H;
        if (particles[i].y > H)  particles[i].y = 0;
    }
}
void draw_particles(SDL_Renderer *r) {
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    for (int i = 0; i < MAX_PARTICLES; i++) {
        int px = (int)particles[i].x;
        int py = (int)particles[i].y;
        int  s = particles[i].size;
        Uint8 a = (Uint8)particles[i].alpha;
        if (particles[i].type == 0) {
            /* Losange */
            SDL_SetRenderDrawColor(r, COL_GOLD.r, COL_GOLD.g, COL_GOLD.b, a);
            SDL_RenderDrawPoint(r, px, py-s);
            SDL_RenderDrawPoint(r, px-s, py);
            SDL_RenderDrawPoint(r, px+s, py);
            SDL_RenderDrawPoint(r, px, py+s);
        } else {
            /* Carré */
            SDL_SetRenderDrawColor(r, COL_BLUE.r, COL_BLUE.g, COL_BLUE.b, a);
            SDL_Rect sq = {px-s, py-s, s*2, s*2};
            SDL_RenderFillRect(r, &sq);
        }
    }
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}

/* ── Grille hexagonale en fond ───────────────────────────────────────────── */
void draw_hex_grid(SDL_Renderer *r) {
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 60, 66, 110, 18);
    int step = 60;
    for (int gy = NAV_H; gy < H; gy += step) {
        int offset = ((gy / step) % 2) * (step / 2);
        for (int gx = -step + offset; gx < W + step; gx += step) {
            /* Hexagone simplifié (6 lignes) */
            int s = 26;
            int pts_x[] = {gx, gx+s, gx+s, gx, gx-s, gx-s, gx};
            int pts_y[] = {gy-s*2/3, gy-s/3, gy+s/3, gy+s*2/3, gy+s/3, gy-s/3, gy-s*2/3};
            for (int k = 0; k < 6; k++)
                SDL_RenderDrawLine(r, pts_x[k], pts_y[k], pts_x[k+1], pts_y[k+1]);
        }
    }
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}

/* ── Carte bancaire stylisée ─────────────────────────────────────────────── */
void draw_bank_card(SDL_Renderer *r, TTF_Font *flabel, TTF_Font *fval,
                    TTF_Font *fsmall, Compte *c, int x, int y, int w, int h) {
    /* Ombre portée */
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 0,0,0,80);
    for (int i = 1; i <= 6; i++) {
        SDL_Rect sh = {x+i, y+i, w, h};
        SDL_RenderFillRect(r, &sh);
    }
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);

    /* Corps de la carte : dégradé bleu-violet */
    SDL_Color card_top = {28, 32, 80, 255};
    SDL_Color card_bot = {60, 20, 100, 255};
    SDL_Rect card_rect = {x, y, w, h};
    draw_gradient_rect(r, card_rect, card_top, card_bot);
    draw_rounded_border(r, card_rect, 14, COL_BORDER2, 1);

    /* Motif décoratif : deux cercles transparents */
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 255, 255, 255, 8);
    for (int dx=-80;dx<=80;dx++) for(int dy=-80;dy<=80;dy++)
        if(dx*dx+dy*dy<=6400) SDL_RenderDrawPoint(r, x+w-60+dx, y+h-30+dy);
    SDL_SetRenderDrawColor(r, 255, 255, 255, 6);
    for (int dx=-60;dx<=60;dx++) for(int dy=-60;dy<=60;dy++)
        if(dx*dx+dy*dy<=3600) SDL_RenderDrawPoint(r, x+w-30+dx, y+h-50+dy);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);

    /* Bande lumineuse en haut */
    SDL_SetRenderDrawColor(r, 255, 255, 255, 15);
    SDL_Rect shine = {x, y, w, 4};
    SDL_RenderFillRect(r, &shine);

    /* Puce (rectangle doré) */
    SDL_Rect chip = {x+20, y+28, 42, 32};
    draw_rounded_rect(r, chip, 4, (SDL_Color){200, 160, 30, 255});
    SDL_SetRenderDrawColor(r, 180, 140, 20, 255);
    SDL_RenderDrawLine(r, chip.x+14, chip.y, chip.x+14, chip.y+chip.h);
    SDL_RenderDrawLine(r, chip.x+28, chip.y, chip.x+28, chip.y+chip.h);
    SDL_RenderDrawLine(r, chip.x, chip.y+10, chip.x+chip.w, chip.y+10);
    SDL_RenderDrawLine(r, chip.x, chip.y+22, chip.x+chip.w, chip.y+22);

    /* Réseau (cercles à droite) */
    SDL_SetRenderDrawColor(r, COL_GOLD.r, COL_GOLD.g, COL_GOLD.b, 160);
    int cx1 = x+w-36, cx2 = x+w-20, cy_c = y+28;
    int R = 16;
    for (int dx=-R;dx<=R;dx++) for(int dy=-R;dy<=R;dy++)
        if((dx*dx+dy*dy<=R*R) && (dx*dx+dy*dy>=(R-2)*(R-2)))
            SDL_RenderDrawPoint(r, cx1+dx, cy_c+dy);
    for (int dx=-R;dx<=R;dx++) for(int dy=-R;dy<=R;dy++)
        if((dx*dx+dy*dy<=R*R) && (dx*dx+dy*dy>=(R-2)*(R-2)))
            SDL_RenderDrawPoint(r, cx2+dx, cy_c+dy);

    /* Numéro de carte (format groupes de 4) */
    char num_fmt[20] = "";
    if (strlen(c->numero) >= 12) {
        snprintf(num_fmt, sizeof(num_fmt), "%c%c%c%c  %c%c%c%c  %c%c%c%c",
            c->numero[0], c->numero[1], c->numero[2], c->numero[3],
            c->numero[4], c->numero[5], c->numero[6], c->numero[7],
            c->numero[8], c->numero[9], c->numero[10],c->numero[11]);
    }
    draw_text(r, fval, num_fmt, x+20, y+76, (SDL_Color){210,220,255,255});

    /* Titulaire */
    draw_text(r, fsmall, "TITULAIRE", x+20, y+h-40, (SDL_Color){150,158,200,255});
    char fullname[130];
    snprintf(fullname, sizeof(fullname), "%s %s",
             c->prenom[0] ? c->prenom : "?",
             c->nom[0] ? c->nom : "?");
    draw_text(r, flabel, fullname, x+20, y+h-24, (SDL_Color){220,228,255,255});

    /* Solde en haut à droite */
    draw_text(r, fsmall, "SOLDE", x+w-120, y+h-40, (SDL_Color){150,158,200,255});
    char solde_s[32];
    snprintf(solde_s, sizeof(solde_s), "%.2f EUR", c->solde);
    SDL_Color sc = c->solde >= 0 ? COL_GREEN : COL_RED;
    draw_text(r, flabel, solde_s, x+w-120, y+h-24, sc);

    /* Logo banque */
    draw_text(r, flabel, "BankSim", x+w-90, y+8, (SDL_Color){230,210,70,200});
    draw_text(r, fsmall, "ULTRA",   x+w-68, y+24, (SDL_Color){180,160,50,160});
}

/* ── Mini graphique historique (sparkline) ───────────────────────────────── */
void draw_sparkline(SDL_Renderer *r, Compte *c, int x, int y, int w, int h) {
    if (!c || c->nb_transactions < 2) {
        SDL_SetRenderDrawColor(r, COL_TEXT3.r, COL_TEXT3.g, COL_TEXT3.b, 100);
        SDL_RenderDrawLine(r, x, y+h/2, x+w, y+h/2);
        return;
    }

    /* Trouver min/max */
    float mn = c->history[0].solde_apres;
    float mx = mn;
    for (int i = 1; i < c->nb_transactions; i++) {
        if (c->history[i].solde_apres < mn) mn = c->history[i].solde_apres;
        if (c->history[i].solde_apres > mx) mx = c->history[i].solde_apres;
    }
    if (mx - mn < 1.0f) mx = mn + 1.0f;

    float step_x = (float)w / (c->nb_transactions - 1);

    /* Remplissage sous la courbe */
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    for (int i = 0; i < c->nb_transactions - 1; i++) {
        float p1y = y + h - h * (c->history[i].solde_apres - mn) / (mx - mn);
        float p2y = y + h - h * (c->history[i+1].solde_apres - mn) / (mx - mn);
        float p1x = x + i * step_x;
        float p2x = x + (i+1) * step_x;
        for (int lx = (int)p1x; lx < (int)p2x; lx++) {
            float t  = (lx - p1x) / (p2x - p1x);
            float ly = p1y + t*(p2y - p1y);
            SDL_SetRenderDrawColor(r, COL_GREEN.r, COL_GREEN.g, COL_GREEN.b, 30);
            SDL_RenderDrawLine(r, lx, (int)ly, lx, y+h);
        }
    }

    /* Ligne de la courbe */
    SDL_SetRenderDrawColor(r, COL_GREEN.r, COL_GREEN.g, COL_GREEN.b, 220);
    for (int i = 0; i < c->nb_transactions - 1; i++) {
        int x1 = x + (int)(i * step_x);
        int y1 = y + h - (int)(h * (c->history[i].solde_apres - mn) / (mx - mn));
        int x2 = x + (int)((i+1) * step_x);
        int y2 = y + h - (int)(h * (c->history[i+1].solde_apres - mn) / (mx - mn));
        SDL_RenderDrawLine(r, x1, y1, x2, y2);
        /* Point */
        SDL_SetRenderDrawColor(r, COL_GREEN.r, COL_GREEN.g, COL_GREEN.b, 255);
        SDL_Rect dot = {x2-2, y2-2, 4, 4};
        SDL_RenderFillRect(r, &dot);
    }
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}

/* ── Indicateur PIN visuel ───────────────────────────────────────────────── */
void draw_pin_indicator(SDL_Renderer *r, int cx, int y, int len, int max_len) {
    int spacing = 28;
    int total_w = (max_len - 1) * spacing;
    int start_x = cx - total_w / 2;
    for (int i = 0; i < max_len; i++) {
        int px = start_x + i * spacing;
        if (i < len) {
            draw_filled_circle(r, px, y, 8, COL_GOLD);
        } else {
            /* Cercle creux */
            for (int a = 0; a < 360; a++) {
                float rad = a * 3.14159f / 180.0f;
                SDL_SetRenderDrawColor(r, COL_BORDER2.r, COL_BORDER2.g, COL_BORDER2.b, 200);
                SDL_RenderDrawPoint(r, px + (int)(8*cosf(rad)), y + (int)(8*sinf(rad)));
            }
        }
    }
}

/* ── Barre de session premium ────────────────────────────────────────────── */
void draw_session_bar(SDL_Renderer *r, TTF_Font *fsmall, TTF_Font *fbtn,
                      int *logout_btn_x) {
    *logout_btn_x = -1;
    if (!login_ok) return;
    Compte *c = chercherCompte(compte_connecte);
    if (!c) return;

    SDL_Rect bar = {W - 340, 4, 330, 80};
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_Color bar_bg = {6, 20, 14, 230};
    draw_rounded_rect(r, bar, 12, bar_bg);
    SDL_SetRenderDrawColor(r, COL_GREEN.r, COL_GREEN.g, COL_GREEN.b, 100);
    SDL_RenderDrawRect(r, &bar);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);

    /* Point vert pulsant */
    draw_filled_circle(r, W-328, 22, 5, COL_GREEN);
    draw_filled_circle(r, W-328, 22, 3, (SDL_Color){200,255,230,255});

    /* Nom */
    char session[96];
    snprintf(session, sizeof(session), "%s %s", c->prenom, c->nom);
    draw_text(r, fbtn, session, W-316, 10, COL_GREEN);

    /* Numéro */
    char num_s[24];
    snprintf(num_s, sizeof(num_s), "N° %s", c->numero);
    draw_text(r, fsmall, num_s, W-316, 32, COL_TEXT2);

    /* Solde */
    char solde_s[32];
    snprintf(solde_s, sizeof(solde_s), "%.2f EUR", c->solde);
    draw_text(r, fbtn, solde_s, W-316, 52, COL_GOLD);

    /* Bouton déconnexion */
    SDL_Rect btn_out = {W-84, 28, 74, 30};
    *logout_btn_x = W-84;
    draw_rounded_rect(r, btn_out, 8, COL_RED_DIM);
    SDL_SetRenderDrawColor(r, COL_RED.r, COL_RED.g, COL_RED.b, 160);
    SDL_RenderDrawRect(r, &btn_out);
    draw_text_centered(r, fsmall, "Quitter", W-47, 34, COL_RED);
}

/* ── Bouton de navigation ────────────────────────────────────────────────── */
void draw_nav_button(SDL_Renderer *r, TTF_Font *font,
                     const char *label, int x, int y, int w, int selected) {
    SDL_Rect btn = {x, y, w, 36};
    if (selected) {
        draw_gradient_rect(r, btn,
            (SDL_Color){200,150,20,255},
            (SDL_Color){150,100,5,255});
        draw_rounded_border(r, btn, 8, COL_GOLD2, 1);
        SDL_Color tc = {10, 12, 20, 255};
        draw_text_centered(r, font, label, x + w/2, y + (36-15)/2, tc);
    } else {
        draw_rounded_rect(r, btn, 8, COL_CARD2);
        SDL_SetRenderDrawColor(r, COL_BORDER.r, COL_BORDER.g, COL_BORDER.b, 100);
        SDL_RenderDrawRect(r, &btn);
        draw_text_centered(r, font, label, x + w/2, y + (36-15)/2, COL_TEXT2);
    }
}

/* ────────────────────────────────────────────────────────────────────────── */
/*                              MAIN                                          */
/* ────────────────────────────────────────────────────────────────────────── */
int main(int argc, char *argv[]) {
	
    (void)argc; (void)argv;

    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();

    SDL_Window *window = SDL_CreateWindow(
        "BankSim ULTRA — Simulateur Bancaire Premium",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, W, H, 0);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    /* Polices */
    const char *fp = "arial.ttf";
    TTF_Font *f_title = TTF_OpenFont(fp, 24);
    TTF_Font *f_label = TTF_OpenFont(fp, 12);
    TTF_Font *f_input = TTF_OpenFont(fp, 17);
    TTF_Font *f_btn   = TTF_OpenFont(fp, 15);
    TTF_Font *f_small = TTF_OpenFont(fp, 12);
    TTF_Font *f_big   = TTF_OpenFont(fp, 30);
    TTF_Font *f_card  = TTF_OpenFont(fp, 16);
    TTF_Font *f_huge  = TTF_OpenFont(fp, 36);

    if (!f_title||!f_label||!f_input||!f_btn||!f_small||!f_big||!f_card||!f_huge) {
        printf("Erreur police : %s\n", TTF_GetError());
        return 1;
    }

    charger_csv();
    init_particles();

    /* ── États ── */
    int ecran        = ECRAN_ACCUEIL;
    int scroll       = 0;
    int active_field = 0;
    int creer_field  = 0;
    int confirm_supp = 0;
    int logout_btn_x = -1;

    char input_numero[13]      = "";
    char input_code[5]         = "";
    char input_montant[20]     = "";
    char input_destinataire[13]= "";
    char inp_nom[64]           = "";
    char inp_prenom[64]        = "";
    char inp_nais[16]          = "";
    char inp_email[128]        = "";
    char last_num[13]          = "";
    char last_code[5]          = "";

    SDL_StartTextInput();
    int running = 1;
    SDL_Event event;

    /* Contenu navigation */
    const char *nav_labels[] = {
        "Accueil","Comptes","Ouvrir","Connexion",
        "Depot","Retrait","Virement","Mon Compte"
    };
    int nav_ecrans[] = {
        ECRAN_ACCUEIL, ECRAN_LISTE, ECRAN_CREER, ECRAN_LOGIN,
        ECRAN_DEPOT,   ECRAN_RETRAIT, ECRAN_VIREMENT, ECRAN_MONCOMPTE
    };
    int nav_n = 8;

    /* ── Boucle principale ── */
    while (running) {
        Uint32 ticks = SDL_GetTicks();

        /* ── Événements ── */
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = 0;

            if (event.type == SDL_MOUSEWHEEL && ecran == ECRAN_LISTE)
                scroll -= event.wheel.y * 30;
            if (scroll < 0) scroll = 0;

            if (event.type == SDL_MOUSEBUTTONDOWN) {
                int mx = event.button.x, my = event.button.y;

                /* Déconnexion */
                if (login_ok && logout_btn_x > 0 &&
                    mx >= logout_btn_x && mx <= logout_btn_x+74 &&
                    my >= 28 && my <= 58) {
                    deconnecter();
                    ecran = ECRAN_ACCUEIL;
                    set_message("Deconnexion reussie. A bientot !", 1);
                    input_numero[0] = '\0'; input_code[0] = '\0';
                    continue;
                }

                /* Navigation */
                int nw = (W - 20) / nav_n - 3;
                for (int i = 0; i < nav_n; i++) {
                    int nx = 10 + i*(nw+3);
                    if (mx >= nx && mx <= nx+nw && my >= 90 && my <= 126) {
                        ecran = nav_ecrans[i];
                        scroll = 0; active_field = 0; creer_field = 0;
                        confirm_supp = 0;
                        input_montant[0] = '\0';
                        input_destinataire[0] = '\0';
                        last_num[0] = '\0'; last_code[0] = '\0';
                    }
                }

                /* ── Login clics ── */
                if (ecran == ECRAN_LOGIN) {
                    int bx = (W-460)/2, fw = 460;
                    if (mx>=bx && mx<=bx+fw && my>=230 && my<=284) active_field=0;
                    if (mx>=bx && mx<=bx+fw && my>=320 && my<=374) active_field=1;
                    SDL_Rect btn = {W/2-130, 410, 260, 56};
                    if (mx>=btn.x && mx<=btn.x+btn.w && my>=btn.y && my<=btn.y+btn.h) {
                        if (verifierLogin(input_numero, input_code)) {
                            set_message("Connexion reussie ! Bienvenue.", 1);
                            input_numero[0]='\0'; input_code[0]='\0';
                            ecran = ECRAN_ACCUEIL;
                        } else {
                            set_message("Numero ou code incorrect !", 0);
                            input_code[0]='\0';
                        }
                    }
                }

                /* ── Créer ── */
                if (ecran == ECRAN_CREER) {
                    int bx = 80, fw = W-160;
                    int fy[] = {210, 290, 370, 450};
                    for (int i = 0; i < 4; i++)
                        if (mx>=bx && mx<=bx+fw && my>=fy[i] && my<=fy[i]+54)
                            creer_field = i;
                    SDL_Rect btn = {W/2-140, 530, 280, 56};
                    if (mx>=btn.x && mx<=btn.x+btn.w && my>=btn.y && my<=btn.y+btn.h) {
                        if (!strlen(inp_nom)||!strlen(inp_prenom))
                            set_message("Nom et prenom sont obligatoires !", 0);
                        else if (!strlen(inp_email))
                            set_message("Adresse email obligatoire !", 0);
                        else {
                            const char *num = creerCompte(inp_nom,inp_prenom,inp_nais,inp_email);
                            if (num) {
                                Compte *cc = chercherCompte(num);
                                strncpy(last_num, num, 12);
                                if (cc) strncpy(last_code, cc->code_acces, 4);
                                char buf[160];
                                snprintf(buf, sizeof(buf),
                                    "Compte %s cree ! Code : %s  (conservez-le !)", num, last_code);
                                set_message(buf, 1);
                                inp_nom[0]=inp_prenom[0]=inp_nais[0]=inp_email[0]='\0';
                                creer_field=0;
                            } else set_message("Limite de comptes atteinte !", 0);
                        }
                    }
                }

                /* ── Dépôt ── */
                if (ecran == ECRAN_DEPOT && login_ok) {
                    SDL_Rect btn = {W/2-140, 340, 280, 56};
                    if (mx>=btn.x && mx<=btn.x+btn.w && my>=btn.y && my<=btn.y+btn.h) {
                        int res = depot(atof(input_montant));
                        if (res==1) set_message("Depot effectue avec succes !", 1);
                        else if (res==-2) set_message("Montant invalide !", 0);
                        else set_message("Erreur lors du depot.", 0);
                        input_montant[0]='\0';
                    }
                }

                /* ── Retrait ── */
                if (ecran == ECRAN_RETRAIT && login_ok) {
                    SDL_Rect btn = {W/2-140, 340, 280, 56};
                    if (mx>=btn.x && mx<=btn.x+btn.w && my>=btn.y && my<=btn.y+btn.h) {
                        int res = retrait(atof(input_montant));
                        if (res==1) set_message("Retrait effectue avec succes !", 1);
                        else if (res==-2) set_message("Montant invalide !", 0);
                        else if (res==-3) set_message("Solde insuffisant !", 0);
                        else set_message("Erreur lors du retrait.", 0);
                        input_montant[0]='\0';
                    }
                }

                /* ── Virement ── */
                if (ecran == ECRAN_VIREMENT && login_ok) {
                    int bx=80, fw=W-160;
                    if (mx>=bx && mx<=bx+fw && my>=248 && my<=302) active_field=0;
                    if (mx>=bx && mx<=bx+fw && my>=338 && my<=392) active_field=1;
                    SDL_Rect btn = {W/2-140, 420, 280, 56};
                    if (mx>=btn.x && mx<=btn.x+btn.w && my>=btn.y && my<=btn.y+btn.h) {
                        int res = virement(input_destinataire, atof(input_montant));
                        if (res==1) set_message("Virement effectue avec succes !", 1);
                        else if (res==-1) set_message("Compte destinataire introuvable !", 0);
                        else if (res==-2) set_message("Montant invalide !", 0);
                        else if (res==-3) set_message("Solde insuffisant !", 0);
                        else if (res==-5) set_message("Virement vers soi-meme impossible !", 0);
                        else set_message("Erreur de virement.", 0);
                        input_destinataire[0]='\0'; input_montant[0]='\0';
                    }
                }

                /* ── Mon Compte ── */
                if (ecran == ECRAN_MONCOMPTE && login_ok) {
                    SDL_Rect btn_supp = {W/2-130, H-120, 260, 48};
                    if (mx>=btn_supp.x && mx<=btn_supp.x+btn_supp.w &&
                        my>=btn_supp.y && my<=btn_supp.y+btn_supp.h) {
                        if (!confirm_supp) {
                            confirm_supp=1;
                            set_message("Cliquez a nouveau pour confirmer la suppression !", 0);
                        } else {
                            if (supprimerCompte()==1) {
                                set_message("Compte supprime definitivement.", 1);
                                ecran=ECRAN_ACCUEIL; confirm_supp=0;
                            }
                        }
                    }
                }
            }

            /* ── Clavier ── */
            if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_TAB) {
                    if (ecran==ECRAN_LOGIN)   active_field=(active_field+1)%2;
                    if (ecran==ECRAN_CREER)   creer_field=(creer_field+1)%4;
                    if (ecran==ECRAN_VIREMENT&&login_ok) active_field=(active_field+1)%2;
                }
                if (event.key.keysym.sym == SDLK_BACKSPACE) {
                    char *t = NULL;
                    if (ecran==ECRAN_LOGIN) t=(active_field==0)?input_numero:input_code;
                    if ((ecran==ECRAN_DEPOT||ecran==ECRAN_RETRAIT)&&login_ok) t=input_montant;
                    if (ecran==ECRAN_VIREMENT&&login_ok)
                        t=(active_field==0)?input_destinataire:input_montant;
                    if (ecran==ECRAN_CREER) {
                        char *fields[]={inp_nom,inp_prenom,inp_nais,inp_email};
                        t=fields[creer_field];
                    }
                    if (t) { int l=strlen(t); if(l>0) t[l-1]='\0'; }
                }
            }

            if (event.type == SDL_TEXTINPUT) {
                char c = event.text.text[0];
                if (ecran==ECRAN_LOGIN) {
                    if (active_field==0&&c>='0'&&c<='9'&&strlen(input_numero)<12)
                        strncat(input_numero, event.text.text, 1);
                    if (active_field==1&&c>='0'&&c<='9'&&strlen(input_code)<4)
                        strncat(input_code, event.text.text, 1);
                }
                if ((ecran==ECRAN_DEPOT||ecran==ECRAN_RETRAIT)&&login_ok&&
                    ((c>='0'&&c<='9')||c=='.')&&strlen(input_montant)<14)
                    strncat(input_montant, event.text.text, 1);
                if (ecran==ECRAN_VIREMENT&&login_ok) {
                    if (active_field==0&&c>='0'&&c<='9'&&strlen(input_destinataire)<12)
                        strncat(input_destinataire, event.text.text, 1);
                    if (active_field==1&&((c>='0'&&c<='9')||c=='.')&&strlen(input_montant)<14)
                        strncat(input_montant, event.text.text, 1);
                }
                if (ecran==ECRAN_CREER) {
                    if (creer_field==0&&strlen(inp_nom)<63)    strncat(inp_nom,event.text.text,1);
                    if (creer_field==1&&strlen(inp_prenom)<63) strncat(inp_prenom,event.text.text,1);
                    if (creer_field==2) {
                        int l=strlen(inp_nais);
                        if (c>='0'&&c<='9'&&l<10){
                            if(l==2||l==5) strncat(inp_nais,"/",1);
                            strncat(inp_nais,event.text.text,1);
                        }
                    }
                    if (creer_field==3&&strlen(inp_email)<127) strncat(inp_email,event.text.text,1);
                }
            }
        }

        /* ══════════════════════════════════════════════════════════════
           RENDU
           ══════════════════════════════════════════════════════════════ */
        update_particles();

        /* Fond */
        SDL_SetRenderDrawColor(renderer, COL_BG.r, COL_BG.g, COL_BG.b, 255);
        SDL_RenderClear(renderer);

        /* Particules de fond */
        draw_particles(renderer);

        /* Grille hexagonale subtile */
        draw_hex_grid(renderer);

        /* ── En-tête ── */
        SDL_Rect hdr = {0, 0, W, 88};
        draw_gradient_rect(renderer, hdr,
            (SDL_Color){18, 22, 44, 255},
            (SDL_Color){10, 13, 28, 255});

        /* Ligne dorée bas header */
        SDL_SetRenderDrawColor(renderer, COL_GOLD.r, COL_GOLD.g, COL_GOLD.b, 180);
        SDL_RenderDrawLine(renderer, 0, 87, W, 87);
        SDL_SetRenderDrawColor(renderer, COL_GOLD.r, COL_GOLD.g, COL_GOLD.b, 40);
        SDL_RenderDrawLine(renderer, 0, 88, W, 88);

        /* Logo ◆ + titre */
        draw_text(renderer, f_huge, "◆", 18, 20, COL_GOLD);
        draw_text(renderer, f_big,  "BankSim ULTRA", 70, 18, COL_GOLD);
        draw_text(renderer, f_small,"Simulateur Bancaire Premium", 72, 54, COL_TEXT2);

        /* Session bar */
        draw_session_bar(renderer, f_small, f_btn, &logout_btn_x);

        /* ── Navigation ── */
        int nw = (W - 20) / nav_n - 3;
        for (int i = 0; i < nav_n; i++) {
            int nx = 10 + i*(nw+3);
            int sel = (ecran == nav_ecrans[i]);
            draw_nav_button(renderer, f_small, nav_labels[i], nx, 90, nw, sel);
            /* Badge vert "Mon Compte" si connecté */
            if (i==7 && login_ok && !sel) {
                draw_filled_circle(renderer, nx+nw-8, 95, 4, COL_GREEN);
            }
        }

        /* Zone de contenu */
        int cy_start = NAV_H + 14;

        /* ════════════════════════════════════════════════════════════
           ÉCRAN ACCUEIL
           ════════════════════════════════════════════════════════════ */
        if (ecran == ECRAN_ACCUEIL) {
            draw_text_centered(renderer, f_title, "Tableau de Bord", W/2, cy_start, COL_TEXT);
            draw_separator(renderer, cy_start+28, 40, W-40);

            float total_solde = 0;
            for (int i = 0; i < totalComptes; i++) total_solde += comptes[i].solde;

            /* 4 cartes stat */
            int cw = (W - 100) / 4;
            char v0[32], v1[32], v2[32], v3[32];
            snprintf(v0, 32, "%d", totalComptes);
            snprintf(v1, 32, "%.0f EUR", total_solde);
            snprintf(v2, 32, login_ok ? "Active" : "Hors conn.");
            snprintf(v3, 32, "Data.csv");

            struct { const char *ic; const char *lb; char *vl; SDL_Color cl; } st[] = {
                {"[C]", "Comptes ouverts", v0, COL_BLUE},
                {"[E]", "Masse monetaire", v1, COL_GREEN},
                {"[S]", "Session",         v2, login_ok ? COL_GREEN : COL_TEXT2},
                {"[D]", "Stockage",        v3, COL_GOLD},
            };
            for (int i = 0; i < 4; i++) {
                SDL_Rect card = {24 + i*(cw+8), cy_start+40, cw, 100};
                draw_stat_card(renderer, f_small, f_btn, f_btn,
                               st[i].ic, st[i].lb, st[i].vl, card, st[i].cl);
            }

            /* Si connecté : carte bancaire */
            if (login_ok) {
                Compte *cc = chercherCompte(compte_connecte);
                if (cc) {
                    draw_bank_card(renderer, f_card, f_card, f_small,
                                   cc, 24, cy_start+156, 420, 160);

                    /* Sparkline à droite */
                    SDL_Rect spark_bg = {460, cy_start+156, W-484, 160};
                    draw_rounded_rect(renderer, spark_bg, 14, COL_CARD);
                    SDL_SetRenderDrawColor(renderer, COL_BLUE.r, COL_BLUE.g, COL_BLUE.b, 100);
                    SDL_Rect spark_top = {460, cy_start+156, W-484, 3};
                    SDL_RenderFillRect(renderer, &spark_top);
                    draw_text(renderer, f_small, "Evolution du solde", 476, cy_start+166, COL_TEXT2);
                    draw_sparkline(renderer, cc, 470, cy_start+188, W-504, 110);
                }
            } else {
                /* Bandeau bienvenue */
                SDL_Rect welcome = {24, cy_start+156, W-48, 100};
                draw_gradient_rect(renderer, welcome,
                    (SDL_Color){20,25,52,255}, (SDL_Color){14,18,40,255});
                draw_rounded_border(renderer, welcome, 14, COL_BORDER, 1);
                draw_text_centered(renderer, f_title, "Bienvenue chez BankSim ULTRA",
                                   W/2, cy_start+182, COL_GOLD);
                draw_text_centered(renderer, f_btn,
                                   "Connectez-vous via l'onglet 'Connexion' pour acceder a vos services",
                                   W/2, cy_start+218, COL_TEXT2);
            }

            /* Guide d'utilisation */
            SDL_Rect guide = {24, cy_start+332, W-48, 210};
            draw_rounded_rect(renderer, guide, 14, COL_CARD);
            SDL_SetRenderDrawColor(renderer, COL_GOLD.r, COL_GOLD.g, COL_GOLD.b, 60);
            SDL_Rect g_top = {24, cy_start+332, W-48, 3};
            SDL_RenderFillRect(renderer, &g_top);

            draw_text(renderer, f_btn, "Guide d'utilisation rapide",
                      44, cy_start+346, COL_GOLD);
            draw_separator(renderer, cy_start+370, 44, W-44);

            const char *tips[] = {
                "  [1] Ouvrir : Renseignez vos informations pour creer un nouveau compte bancaire",
                "  [2] Connexion : Numero de compte (12 chiffres) + code secret (4 chiffres)",
                "  [3] Depot / Retrait : Mouvements de fonds sur votre compte connecte",
                "  [4] Virement : Transfert vers un autre compte (numero requis)",
                "  [5] Mon Compte : Historique des transactions, carte virtuelle et gestion du compte",
                "  [i] Toutes les donnees sont sauvegardees automatiquement dans Data.csv",
            };
            for (int i = 0; i < 6; i++)
                draw_text(renderer, f_small, tips[i], 42, cy_start+380+i*25, COL_TEXT2);
        }

        /* ════════════════════════════════════════════════════════════
           ÉCRAN LISTE
           ════════════════════════════════════════════════════════════ */
        if (ecran == ECRAN_LISTE) {
            draw_text_centered(renderer, f_title, "Liste des Comptes", W/2, cy_start, COL_TEXT);
            draw_separator(renderer, cy_start+28, 40, W-40);

            if (totalComptes == 0) {
                draw_text_centered(renderer, f_btn, "Aucun compte enregistre.",
                                   W/2, cy_start+200, COL_TEXT2);
            } else {
                /* En-tête tableau */
                SDL_Rect hdr2 = {20, cy_start+40, W-40, 36};
                draw_gradient_rect(renderer, hdr2,
                    (SDL_Color){44,48,90,255}, (SDL_Color){34,38,70,255});
                draw_text(renderer, f_label, "N COMPTE",      38, cy_start+52, COL_GOLD);
                draw_text(renderer, f_label, "CODE",         206, cy_start+52, COL_GOLD);
                draw_text(renderer, f_label, "TITULAIRE",    275, cy_start+52, COL_GOLD);
                draw_text(renderer, f_label, "EMAIL",        510, cy_start+52, COL_GOLD);
                draw_text(renderer, f_label, "NAISSANCE",    760, cy_start+52, COL_GOLD);
                draw_text(renderer, f_label, "SOLDE",        940, cy_start+52, COL_GOLD);

                int y0 = cy_start+82, rh = 46;
                int max_scroll = totalComptes*rh - (H - NAV_H - 100);
                if (max_scroll < 0) max_scroll = 0;
                if (scroll > max_scroll) scroll = max_scroll;

                for (int i = 0; i < totalComptes; i++) {
                    int y = y0 + i*rh - scroll;
                    if (y < NAV_H+40 || y > H-70) continue;

                    SDL_Color row_bg = i%2==0 ? COL_CARD : COL_CARD2;
                    SDL_Rect row = {20, y, W-40, rh-4};
                    draw_rounded_rect(renderer, row, 6, row_bg);

                    /* Barre latérale de couleur selon solde */
                    SDL_Color lc = comptes[i].solde >= 0 ? COL_GREEN : COL_RED;
                    SDL_Rect ind = {20, y, 4, rh-4};
                    SDL_SetRenderDrawColor(renderer, lc.r, lc.g, lc.b, 255);
                    SDL_RenderFillRect(renderer, &ind);

                    /* Surlignage compte connecté */
                    if (login_ok && strcmp(comptes[i].numero, compte_connecte)==0) {
                        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                        SDL_SetRenderDrawColor(renderer, COL_GOLD.r,COL_GOLD.g,COL_GOLD.b,40);
                        SDL_RenderFillRect(renderer, &row);
                        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
                        SDL_SetRenderDrawColor(renderer, COL_GOLD.r,COL_GOLD.g,COL_GOLD.b,120);
                        SDL_RenderDrawRect(renderer, &row);
                    }

                    draw_text(renderer, f_small, comptes[i].numero, 38, y+14, COL_TEXT);
                    draw_text(renderer, f_small, "****",            206, y+14, COL_TEXT2);

                    char full[130];
                    snprintf(full, sizeof(full), "%s %s",
                             comptes[i].prenom, comptes[i].nom);
                    draw_text(renderer, f_small, full, 275, y+14, COL_TEXT);

                    char em[34];
                    strncpy(em, comptes[i].email, 30); em[30]='\0';
                    if (strlen(comptes[i].email)>30) strcat(em,"...");
                    draw_text(renderer, f_small, em, 510, y+14, COL_TEXT2);

                    draw_text(renderer, f_small,
                              strlen(comptes[i].naissance)?comptes[i].naissance:"—",
                              760, y+14, COL_TEXT2);

                    char buf[32];
                    snprintf(buf, sizeof(buf), "%.2f EUR", comptes[i].solde);
                    draw_text(renderer, f_small, buf, 940, y+14, lc);
                }
            }
        }

        /* ════════════════════════════════════════════════════════════
           ÉCRAN CRÉER
           ════════════════════════════════════════════════════════════ */
        if (ecran == ECRAN_CREER) {
            draw_text_centered(renderer, f_title, "Ouvrir un Nouveau Compte", W/2, cy_start, COL_TEXT);
            draw_separator(renderer, cy_start+28, 40, W-40);

            int bx = 80, fw = W - 160;
            draw_input(renderer, f_label, f_input, "Nom *",
                       inp_nom, bx, cy_start+66, fw, creer_field==0, ticks, 0);
            draw_input(renderer, f_label, f_input, "Prenom *",
                       inp_prenom, bx, cy_start+146, fw, creer_field==1, ticks, 0);
            draw_input(renderer, f_label, f_input, "Date de naissance (JJ/MM/AAAA)",
                       inp_nais, bx, cy_start+226, fw, creer_field==2, ticks, 0);
            draw_input(renderer, f_label, f_input, "Adresse email *",
                       inp_email, bx, cy_start+306, fw, creer_field==3, ticks, 0);

            SDL_Rect btn = {W/2-140, cy_start+390, 280, 56};
            draw_button_gradient(renderer, f_btn, "  Creer mon compte  ", btn,
                COL_GOLD, COL_GOLD3, COL_GOLD2, (SDL_Color){10,12,20,255});

            draw_text_centered(renderer, f_small,
                "Un numero unique (12 chiffres) et un code secret (4 chiffres) seront generes",
                W/2, cy_start+464, COL_TEXT2);

            if (strlen(last_num) > 0) {
                SDL_Rect card = {60, cy_start+484, W-120, 80};
                draw_gradient_rect(renderer, card,
                    (SDL_Color){6,30,18,255}, (SDL_Color){4,20,12,255});
                draw_rounded_border(renderer, card, 12, COL_GREEN, 1);
                draw_text_centered(renderer, f_label,
                    "COMPTE CREE AVEC SUCCES — CONSERVEZ CES INFORMATIONS",
                    W/2, cy_start+496, COL_GREEN);

                char buf[160];
                snprintf(buf, sizeof(buf),
                    "Numero : %s     Code secret : %s", last_num, last_code);
                draw_text_centered(renderer, f_btn, buf, W/2, cy_start+524, COL_GOLD);
            }
        }

        /* ════════════════════════════════════════════════════════════
           ÉCRAN LOGIN
           ════════════════════════════════════════════════════════════ */
        if (ecran == ECRAN_LOGIN) {
            /* Bandeau de connexion */
            SDL_Rect banner = {(W-500)/2, cy_start+10, 500, 60};
            draw_gradient_rect(renderer, banner,
                (SDL_Color){22,28,70,255}, (SDL_Color){14,18,50,255});
            draw_rounded_border(renderer, banner, 14, COL_BLUE, 1);
            draw_text_centered(renderer, f_title, "Connexion Securisee",
                               W/2, cy_start+22, COL_TEXT);
            draw_text_centered(renderer, f_small,
                "Entrez votre numero de compte et votre code secret",
                W/2, cy_start+48, COL_TEXT2);

            int bx = (W-460)/2, fw = 460;
            draw_input(renderer, f_label, f_input,
                       "Numero de compte (12 chiffres)",
                       input_numero, bx, cy_start+96, fw, active_field==0, ticks, 0);
            draw_input(renderer, f_label, f_input,
                       "Code secret (4 chiffres)",
                       input_code, bx, cy_start+186, fw, active_field==1, ticks, 1);

            /* Indicateur PIN visuel */
            int pin_y = cy_start+262;
            draw_text_centered(renderer, f_small, "Code saisi :", W/2, pin_y-16, COL_TEXT2);
            int pin_len = (int)strlen(input_code);
            int spacing = 32;
            int total_pw = 3 * spacing;
            int start_px = W/2 - total_pw/2;
            for (int i = 0; i < 4; i++) {
                int px = start_px + i * spacing;
                if (i < pin_len)
                    draw_filled_circle(renderer, px, pin_y, 9, COL_GOLD);
                else {
                    SDL_SetRenderDrawColor(renderer, COL_BORDER2.r, COL_BORDER2.g, COL_BORDER2.b, 200);
                    for (int a=0;a<360;a++) {
                        float rad=a*3.14159f/180.0f;
                        SDL_RenderDrawPoint(renderer, px+(int)(9*cosf(rad)), pin_y+(int)(9*sinf(rad)));
                    }
                }
            }

            SDL_Rect btn = {W/2-130, cy_start+296, 260, 56};
            draw_button_gradient(renderer, f_btn, "  Se connecter  ", btn,
                COL_GREEN2, COL_GREEN_DIM, COL_GREEN, COL_WHITE);

            /* Astuce */
            SDL_Rect tip = {bx, cy_start+372, fw, 44};
            draw_rounded_rect(renderer, tip, 10, COL_CARD2);
            SDL_SetRenderDrawColor(renderer, COL_BORDER.r, COL_BORDER.g, COL_BORDER.b, 100);
            SDL_RenderDrawRect(renderer, &tip);
            draw_text_centered(renderer, f_small,
                "Retrouvez votre numero et code sur le recepisse de creation de compte",
                W/2, cy_start+387, COL_TEXT2);

            if (login_ok)
                draw_text_centered(renderer, f_btn,
                    "Vous etes deja connecte(e).", W/2, cy_start+432, COL_GREEN);
        }

        /* ════════════════════════════════════════════════════════════
           ÉCRAN DÉPÔT
           ════════════════════════════════════════════════════════════ */
        if (ecran == ECRAN_DEPOT) {
            if (!login_ok) {
                draw_text_centered(renderer, f_title, "Acces refuse", W/2, cy_start+100, COL_RED);
                draw_text_centered(renderer, f_btn, "Connectez-vous via l'onglet Connexion",
                                   W/2, cy_start+140, COL_TEXT2);
            } else {
                draw_text_centered(renderer, f_title, "Effectuer un Depot", W/2, cy_start, COL_TEXT);
                draw_separator(renderer, cy_start+28, 40, W-40);

                Compte *cc = chercherCompte(compte_connecte);
                if (cc) {
                    SDL_Rect info = {60, cy_start+40, W-120, 54};
                    draw_gradient_rect(renderer, info,
                        (SDL_Color){8,34,22,255}, (SDL_Color){4,22,14,255});
                    draw_rounded_border(renderer, info, 10, COL_GREEN, 1);
                    char msg[100];
                    snprintf(msg, sizeof(msg), "Compte : %s   —   Solde actuel : %.2f EUR",
                             cc->numero, cc->solde);
                    draw_text_centered(renderer, f_btn, msg, W/2, cy_start+60, COL_GREEN);
                }

                draw_input(renderer, f_label, f_input, "Montant a deposer (EUR)",
                           input_montant, (W-460)/2, cy_start+116, 460, 1, ticks, 0);

                SDL_Rect btn = {W/2-140, cy_start+196, 280, 56};
                draw_button_gradient(renderer, f_btn, "  Valider le depot  ", btn,
                    COL_GREEN2, COL_GREEN_DIM, COL_GREEN, COL_WHITE);

                /* Infos rapides */
                SDL_Rect quick = {60, cy_start+270, W-120, 100};
                draw_rounded_rect(renderer, quick, 12, COL_CARD);
                draw_text(renderer, f_small, "Informations :", 80, cy_start+282, COL_TEXT2);
                draw_text(renderer, f_small, "  • Dépot minimum : 0.01 EUR", 80, cy_start+302, COL_TEXT3);
                draw_text(renderer, f_small, "  • Aucune limite maximale", 80, cy_start+322, COL_TEXT3);
                draw_text(renderer, f_small, "  • Sauvegarde automatique apres chaque operation", 80, cy_start+342, COL_TEXT3);
            }
        }

        /* ════════════════════════════════════════════════════════════
           ÉCRAN RETRAIT
           ════════════════════════════════════════════════════════════ */
        if (ecran == ECRAN_RETRAIT) {
            if (!login_ok) {
                draw_text_centered(renderer, f_title, "Acces refuse", W/2, cy_start+100, COL_RED);
                draw_text_centered(renderer, f_btn, "Connectez-vous via l'onglet Connexion",
                                   W/2, cy_start+140, COL_TEXT2);
            } else {
                draw_text_centered(renderer, f_title, "Effectuer un Retrait", W/2, cy_start, COL_TEXT);
                draw_separator(renderer, cy_start+28, 40, W-40);

                Compte *cc = chercherCompte(compte_connecte);
                if (cc) {
                    SDL_Rect info = {60, cy_start+40, W-120, 54};
                    draw_gradient_rect(renderer, info,
                        (SDL_Color){34,8,8,255}, (SDL_Color){22,4,4,255});
                    draw_rounded_border(renderer, info, 10, COL_RED, 1);
                    char msg[100];
                    snprintf(msg, sizeof(msg), "Compte : %s   —   Solde disponible : %.2f EUR",
                             cc->numero, cc->solde);
                    draw_text_centered(renderer, f_btn, msg, W/2, cy_start+60, COL_RED);
                }

                draw_input(renderer, f_label, f_input, "Montant a retirer (EUR)",
                           input_montant, (W-460)/2, cy_start+116, 460, 1, ticks, 0);

                SDL_Rect btn = {W/2-140, cy_start+196, 280, 56};
                draw_button_gradient(renderer, f_btn, "  Valider le retrait  ", btn,
                    COL_RED2, COL_RED_DIM, COL_RED, COL_WHITE);

                SDL_Rect quick = {60, cy_start+270, W-120, 100};
                draw_rounded_rect(renderer, quick, 12, COL_CARD);
                draw_text(renderer, f_small, "Informations :", 80, cy_start+282, COL_TEXT2);
                draw_text(renderer, f_small, "  • Retrait refuse si solde insuffisant", 80, cy_start+302, COL_TEXT3);
                draw_text(renderer, f_small, "  • Montant minimum : 0.01 EUR", 80, cy_start+322, COL_TEXT3);
                draw_text(renderer, f_small, "  • Sauvegarde automatique apres chaque operation", 80, cy_start+342, COL_TEXT3);
            }
        }

        /* ════════════════════════════════════════════════════════════
           ÉCRAN VIREMENT
           ════════════════════════════════════════════════════════════ */
        if (ecran == ECRAN_VIREMENT) {
            if (!login_ok) {
                draw_text_centered(renderer, f_title, "Acces refuse", W/2, cy_start+100, COL_RED);
                draw_text_centered(renderer, f_btn, "Connectez-vous via l'onglet Connexion",
                                   W/2, cy_start+140, COL_TEXT2);
            } else {
                draw_text_centered(renderer, f_title, "Effectuer un Virement", W/2, cy_start, COL_TEXT);
                draw_separator(renderer, cy_start+28, 40, W-40);

                Compte *cc = chercherCompte(compte_connecte);
                if (cc) {
                    SDL_Rect info = {60, cy_start+40, W-120, 54};
                    draw_gradient_rect(renderer, info,
                        (SDL_Color){10,10,40,255}, (SDL_Color){6,6,28,255});
                    draw_rounded_border(renderer, info, 10, COL_BLUE, 1);
                    char msg[100];
                    snprintf(msg, sizeof(msg), "Compte source : %s   —   Solde : %.2f EUR",
                             cc->numero, cc->solde);
                    draw_text_centered(renderer, f_btn, msg, W/2, cy_start+60, COL_BLUE);
                }

                int bx=80, fw=W-160;
                draw_input(renderer, f_label, f_input,
                           "Numero du compte destinataire (12 chiffres)",
                           input_destinataire, bx, cy_start+116, fw,
                           active_field==0, ticks, 0);
                draw_input(renderer, f_label, f_input,
                           "Montant a virer (EUR)",
                           input_montant, bx, cy_start+200, fw,
                           active_field==1, ticks, 0);

                /* Flèche animée */
                int ax = W/2, ay = cy_start+174;
                SDL_SetRenderDrawColor(renderer, COL_BLUE.r, COL_BLUE.g, COL_BLUE.b, 180);
                SDL_RenderDrawLine(renderer, ax-20, ay, ax+20, ay);
                SDL_RenderDrawLine(renderer, ax+12, ay-6, ax+20, ay);
                SDL_RenderDrawLine(renderer, ax+12, ay+6, ax+20, ay);

                SDL_Rect btn = {W/2-140, cy_start+278, 280, 56};
                draw_button_gradient(renderer, f_btn, "  Valider le virement  ", btn,
                    COL_BLUE2, COL_BLUE_DIM, COL_BLUE, COL_WHITE);

                draw_text_centered(renderer, f_small,
                    "Tab ou Entree pour passer d'un champ a l'autre",
                    W/2, cy_start+348, COL_TEXT2);
            }
        }

        /* ════════════════════════════════════════════════════════════
           ÉCRAN MON COMPTE
           ════════════════════════════════════════════════════════════ */
        if (ecran == ECRAN_MONCOMPTE) {
            if (!login_ok) {
                draw_text_centered(renderer, f_title, "Acces refuse", W/2, cy_start+100, COL_RED);
                draw_text_centered(renderer, f_btn, "Connectez-vous via l'onglet Connexion",
                                   W/2, cy_start+140, COL_TEXT2);
            } else {
                Compte *cc = chercherCompte(compte_connecte);
                draw_text_centered(renderer, f_title, "Mon Compte", W/2, cy_start, COL_TEXT);
                draw_separator(renderer, cy_start+28, 40, W-40);

                if (cc) {
                    /* Côté gauche : carte + infos */
                    draw_bank_card(renderer, f_card, f_card, f_small,
                                   cc, 24, cy_start+40, 380, 150);

                    /* Infos détaillées */
                    SDL_Rect det = {24, cy_start+204, 380, 210};
                    draw_rounded_rect(renderer, det, 14, COL_CARD);
                    SDL_SetRenderDrawColor(renderer, COL_PURPLE.r, COL_PURPLE.g, COL_PURPLE.b, 150);
                    SDL_Rect det_top = {24, cy_start+204, 380, 3};
                    SDL_RenderFillRect(renderer, &det_top);

                    /* Avatar */
                    int avx = 50, avy = cy_start+230;
                    SDL_Color av_cols[] = {COL_BLUE, COL_GREEN, COL_PURPLE, COL_GOLD};
                    int av_idx = (cc->prenom[0] + cc->nom[0]) % 4;
                    draw_filled_circle(renderer, avx, avy, 22, av_cols[av_idx]);
                    char initials[3] = {cc->prenom[0], cc->nom[0], '\0'};
                    draw_text_centered(renderer, f_btn, initials, avx, avy-8, COL_WHITE);

                    int ix = 82, iy = cy_start+214, lh = 32;
                    char buf[200];

                    draw_text(renderer, f_small, "Titulaire", ix, iy, COL_TEXT2);
                    snprintf(buf, sizeof(buf), "%s %s", cc->prenom, cc->nom);
                    draw_text(renderer, f_btn, buf, ix, iy+14, COL_TEXT);

                    draw_text(renderer, f_small, "Numero", ix, iy+lh, COL_TEXT2);
                    draw_text(renderer, f_btn, cc->numero, ix, iy+lh+14, COL_GOLD);

                    draw_text(renderer, f_small, "Email", ix, iy+lh*2, COL_TEXT2);
                    draw_text(renderer, f_btn, cc->email, ix, iy+lh*2+14, COL_TEXT);

                    draw_text(renderer, f_small, "Naissance", ix, iy+lh*3, COL_TEXT2);
                    draw_text(renderer, f_btn,
                              strlen(cc->naissance)?cc->naissance:"—",
                              ix, iy+lh*3+14, COL_TEXT);

                    draw_text(renderer, f_small, "Code", ix, iy+lh*4, COL_TEXT2);
                    draw_text(renderer, f_btn, "****", ix, iy+lh*4+14, COL_TEXT3);

                    /* Solde grand */
                    SDL_Rect solde_b = {24, cy_start+424, 380, 60};
                    draw_gradient_rect(renderer, solde_b,
                        (SDL_Color){8,40,26,255}, (SDL_Color){4,24,16,255});
                    draw_rounded_border(renderer, solde_b, 10, COL_GREEN, 1);
                    draw_text_centered(renderer, f_small, "SOLDE DISPONIBLE",
                                       24+190, cy_start+432, COL_TEXT2);
                    snprintf(buf, sizeof(buf), "%.2f EUR", cc->solde);
                    draw_text_centered(renderer, f_title, buf,
                                       24+190, cy_start+450, COL_GREEN);

                    /* ── Historique à droite ── */
                    SDL_Rect hist_bg = {420, cy_start+40, W-444, 444};
                    draw_rounded_rect(renderer, hist_bg, 14, COL_CARD);
                    SDL_SetRenderDrawColor(renderer, COL_GOLD.r, COL_GOLD.g, COL_GOLD.b, 100);
                    SDL_Rect hist_top = {420, cy_start+40, W-444, 3};
                    SDL_RenderFillRect(renderer, &hist_top);

                    draw_text(renderer, f_btn, "Historique des transactions",
                              436, cy_start+52, COL_GOLD);
                    draw_separator(renderer, cy_start+76, 436, W-28);

                    if (cc->nb_transactions == 0) {
                        draw_text_centered(renderer, f_small, "Aucune transaction pour ce compte.",
                                           420 + (W-444)/2, cy_start+240, COL_TEXT2);
                    } else {
                        /* En-têtes */
                        draw_text(renderer, f_label, "TYPE",   438, cy_start+84, COL_TEXT2);
                        draw_text(renderer, f_label, "DATE",   530, cy_start+84, COL_TEXT2);
                        draw_text(renderer, f_label, "DETAIL", 680, cy_start+84, COL_TEXT2);
                        draw_text(renderer, f_label, "MONTANT",840, cy_start+84, COL_TEXT2);
                        draw_text(renderer, f_label, "SOLDE",  970, cy_start+84, COL_TEXT2);

                        int max_show = 10;
                        int start_i = cc->nb_transactions - max_show;
                        if (start_i < 0) start_i = 0;

                        for (int i = start_i; i < cc->nb_transactions; i++) {
                            Transaction *tr = &cc->history[i];
                            int row_idx = i - start_i;
                            int ty = cy_start + 100 + row_idx * 34;

                            SDL_Rect row = {428, ty, W-456, 30};
                            SDL_Color rb = row_idx%2==0 ? COL_CARD2 : COL_CARD3;
                            draw_rounded_rect(renderer, row, 4, rb);

                            /* Couleur selon type */
                            SDL_Color tc2 = COL_TEXT;
                            SDL_Color mc  = COL_TEXT;
                            if (strncmp(tr->type,"DEPOT",5)==0)      { tc2=COL_GREEN;  mc=COL_GREEN;  }
                            if (strncmp(tr->type,"RETRAIT",7)==0)     { tc2=COL_RED;    mc=COL_RED;    }
                            if (strncmp(tr->type,"VIREMENT-",9)==0)   { tc2=COL_BLUE;   mc=COL_RED;    }
                            if (strncmp(tr->type,"VIREMENT+",9)==0)   { tc2=COL_BLUE;   mc=COL_GREEN;  }

                            /* Indicateur latéral */
                            SDL_SetRenderDrawColor(renderer, tc2.r, tc2.g, tc2.b, 200);
                            SDL_Rect ind2 = {428, ty, 3, 30};
                            SDL_RenderFillRect(renderer, &ind2);

                            draw_text(renderer, f_small, tr->type,   438, ty+8, tc2);
                            draw_text(renderer, f_small, tr->date,   530, ty+8, COL_TEXT2);

                            char det[28];
                            strncpy(det, tr->detail, 24); det[24]='\0';
                            if (strlen(tr->detail)>24) strcat(det,"...");
                            draw_text(renderer, f_small, det,        680, ty+8, COL_TEXT2);

                            char am[32];
                            snprintf(am, sizeof(am), "%.2f EUR", tr->montant);
                            draw_text(renderer, f_small, am,         840, ty+8, mc);

                            char sl[32];
                            snprintf(sl, sizeof(sl), "%.2f", tr->solde_apres);
                            draw_text(renderer, f_small, sl,         970, ty+8, COL_TEXT2);
                        }
                    }

                    /* Bouton suppression */
                    SDL_Rect btn_supp = {W/2-130, H-116, 260, 48};
                    SDL_Color sc2 = confirm_supp ? COL_RED : COL_CARD3;
                    draw_rounded_rect(renderer, btn_supp, 10, sc2);
                    SDL_SetRenderDrawColor(renderer,
                        COL_RED.r, COL_RED.g, COL_RED.b, confirm_supp ? 255 : 120);
                    SDL_RenderDrawRect(renderer, &btn_supp);
                    draw_text_centered(renderer, f_btn,
                        confirm_supp ? "Confirmer la suppression definitive ?"
                                     : "Supprimer mon compte",
                        W/2, H-100, COL_RED);
                }
            }
        }

        /* ── Message flash ── */
        draw_message(renderer, f_btn, f_small, ticks);

        SDL_RenderPresent(renderer);
    }

    /* Nettoyage */
    SDL_StopTextInput();
    TTF_CloseFont(f_title);
    TTF_CloseFont(f_label);
    TTF_CloseFont(f_input);
    TTF_CloseFont(f_btn);
    TTF_CloseFont(f_small);
    TTF_CloseFont(f_big);
    TTF_CloseFont(f_card);
    TTF_CloseFont(f_huge);
    TTF_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return EXIT_SUCCESS;
}
