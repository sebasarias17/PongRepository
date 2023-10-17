import socket
import threading
import pygame
import setup
from pygame.locals import QUIT

FPS = setup.FPS
WHITE = setup.BGCOLOR
BLACK = setup.TXTCOLOR

SERVER_IP = setup.SERVER_IP
SERVER_PORT = setup.SERVER_PORT
BUFFER_SIZE = setup.BUFFER_SIZE

SCREENW = setup.WIDTH
SCREENH = setup.HEIGTH

start_game = False
score_player1 = 0
score_player2 = 0
running = True

class ball:
    def __init__(self, img):
        self.img = pygame.image.load(img).convert_alpha()
        self.x = 380
        self.y = 280

class playerSkin:
    def __init__(self, img):
        self.img = pygame.image.load(img).convert_alpha()
        self.x = 0
        self.y = 250
        self.dir_y = 0
        

pygame.init() #Inicia el juego
window = pygame.display.set_mode((SCREENW, SCREENH)) #Dimension de la pantalla
pygame.display.set_caption("TelePong Champions Edition") 

background_pic = pygame.image.load(setup.FIELD).convert()  #Fondo del juego

ball = ball(setup.BALL) #Creacion de la pelota
player_1 = playerSkin(setup.PLAYER1)  # Creacion del jugador
player_1.x = 60  #Posición en x del jugador 1
player_2 = playerSkin(setup.PLAYER2) # Creacion del jugador
player_2.x = 718 #Posición en x del jugador 2

def refresh_screen_and_positions():
    font = pygame.font.Font(None, 60) #Letras del juego
    window.blit(background_pic, (0, 0))  # Fondo de pantalla seteada
    
    window.blit(ball.img, (ball.x, ball.y)) #Pintar la imagen con sus propiedades
    window.blit(player_1.img, (player_1.x, player_1.y)) #pintar el jugador 1 con sus propiedades
    window.blit(player_2.img, (player_2.x, player_2.y)) #pintar el jugador 2 con sus propiedades
    txt = f"Player 1 {score_player1} : {score_player2} Player 2" #Pintar el marcador del juego
    signal = font.render(txt, False, WHITE)
    window.blit(signal, (800 / 2 - font.size(txt)[0] / 2, 50))
    
    pygame.display.flip()

def listen_for_messages(client_socket): #Metodo que escucha del servidor y nos trae los datos para decodificarlos y mostrarlos
    global start_game, score_player1, score_player2
    while True:
        data, _ = client_socket.recvfrom(BUFFER_SIZE)
        if data:
            data = data.decode()
            if data == "START":
                start_game = True
                print("Los represnetantes de los equipos han llegado.")
                continue
            print("Posiciones: " + data)
            data = data.split()
            ball.x, ball.y, player_1.y, player_2.y, score_player1, score_player2 = map(int, data)

            if not running:
                break
            refresh_screen_and_positions()

def alerts(alert, y_offset=0): #Se usa para cuando se inicia el juego y mostrar los mensajes que seteamos antes 
    font = pygame.font.Font(None, 60)
    text_surface = font.render(alert, True, BLACK)
    text_rect = text_surface.get_rect(center=(400, 300 + y_offset))
    window.blit(text_surface, text_rect)
    pygame.display.flip()

def main():
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    
    client_socket.sendto("CONNECT".encode(), (SERVER_IP, SERVER_PORT)) #Le mandamos al servidor la IP y el puerto
    print("Se ha conectado al servidor.")
    
    listener_thread = threading.Thread(target=listen_for_messages, args=(client_socket,))
    listener_thread.daemon = True
    listener_thread.start()
    
    window.fill(WHITE)
    alerts("Bienvenido a Pong Champions Edition!", -20)
    alerts("Esperando otro jugador...", 20)
    while not start_game:
        for event in pygame.event.get():
            if event.type == QUIT:
                pygame.quit()
                return

    global running
    clock = pygame.time.Clock()
    current_movement = "0"

    while running:
        refresh_screen_and_positions()
        if (score_player1 == 5 or score_player2 == 5):
            running = False
            
        for event in pygame.event.get():
            if (event.type == QUIT):
                running = False

            if event.type == pygame.KEYDOWN:
                if event.key == pygame.K_w:
                    current_movement = "-5"
                if event.key == pygame.K_s:
                    current_movement = "5"

            if event.type == pygame.KEYUP:
                if event.key == pygame.K_w or event.key == pygame.K_s:
                    current_movement = "0"

        client_socket.sendto(current_movement.encode(), (SERVER_IP, SERVER_PORT))
            
        pygame.display.flip() #Actualiza la pantalla
        clock.tick(FPS)
        
    window.fill(WHITE)
    if score_player1 == 5:
        alerts("El jugador 1 ha ganado", -20)
        pygame.time.wait(2000)
    elif score_player2 == 5:
        alerts("El jugador 2 ha ganado!", -20)
        pygame.time.wait(2000)
    else:
        alerts("Cerrando en 5 segundos...", 20)
        pygame.time.wait(1000)
        
    for i in range(5):
            window.fill(WHITE)
            alerts(f"Juego terminando en {5-i}", 20)
            pygame.time.wait(1000)

    pygame.quit()
    client_socket.close()

if __name__ == "__main__":
    main()