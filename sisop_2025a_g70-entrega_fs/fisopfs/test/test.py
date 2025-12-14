import subprocess
import difflib
import os
import shutil 


# Colores para imprimir en terminal
class Colors:
    GREEN = '\033[92m'
    RED = '\033[91m'
    RESET = '\033[0m'

def run_shell_block(script):
    """
    Ejecuta un bloque de comandos shell en bash,
    capturando stdout y stderr (pero no los muestra).
    """
    result = subprocess.run(script, shell=True, executable="/bin/bash",
                            stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    return result.returncode

def read_file_safe(path):
    try:
        with open(path, 'r') as f:
            return f.readlines()
    except FileNotFoundError:
        return None

def normalize_lines(lines):
    """
    Normaliza líneas eliminando:
    - caracteres nulos (\0)
    - espacios en blanco al final
    - saltos de línea inconsistentes
    - líneas vacías al final del archivo
    - caracteres basura como '%' al final de líneas
    """
    normalized = []

    for line in lines:
        line = line.replace('\0', '')

        clean = line.rstrip().replace('\r\n', '\n').replace('\r', '\n')

        # Remueve un '%' final si aparece como ruido
        if clean.endswith('%'):
            clean = clean[:-1].rstrip()

        normalized.append(clean)

    # Elimina líneas vacías al final
    while normalized and normalized[-1].strip() == '':
        normalized.pop()

    # Reagrega '\n' uniforme a cada línea
    return [line + '\n' for line in normalized]

def compare_files(expected_path, got_path):
    expected_lines = read_file_safe(expected_path)
    got_lines = read_file_safe(got_path)

    if expected_lines is None or got_lines is None:
        return False, f"No se pudo abrir uno de los archivos: {expected_path} o {got_path}"

    expected_lines = normalize_lines(expected_lines)
    got_lines = normalize_lines(got_lines)

    diff = list(difflib.unified_diff(expected_lines, got_lines,
                                     fromfile=expected_path,
                                     tofile=got_path))

    return (len(diff) == 0), diff

def print_test_result(test_name, description, passed, diff, out_path, err_path):
    if passed:
        print(f"{Colors.GREEN}{test_name} ✓{Colors.RESET}")
    else:
        print(f"{Colors.RED}{test_name} ✗{Colors.RESET}")
        print(f"Descripción: {description}")
        print(f"\nContenido de {out_path}:")
        with open(out_path, 'r') as f:
            print(f.read())
        print(f"\nContenido de {err_path}:")
        with open(err_path, 'r') as f:
            print(f.read())
        print("\nDiferencias:")
        for line in diff:
            print(line, end='')

def test_touch_fisop():
    shell_script = """
    cd ../prueba
    touch p1.txt
    touch p2.txt
    ls 1>../test/received_output/out.txt 2>../test/received_output/err.txt
    cd ../test
    """
    return shell_script, "Verifica creación de archivos con touch fisop"

def test_echo_redir_fisop():
    shell_script = """
    cd ../prueba
    echo "Hola Mundo" > p.txt
    cat p.txt 1>../test/received_output/out.txt 2>../test/received_output/err.txt
    cd ../test
    """
    return shell_script, "Verifica redirección de texto a archivo fisop"

def test_echo_redir_a_archivo_existente_fisop(): 
    shell_script = """
    cd ../prueba
    touch nuevo.txt
    echo "Hola Mundo" > nuevo.txt
    echo "Nuevo Hola Mundo" > nuevo.txt
    cat nuevo.txt 1>../test/received_output/out.txt 2>../test/received_output/err.txt
    cd ../test
    """
    return shell_script, "Verifica redirección de texto a un archivo existente fisop"


def test_mkdir(): 
    shell_script = """
    cd ../prueba
    ls >../test/received_output/out.txt
    mkdir carpeta 2>../test/received_output/err.txt
    ls >>../test/received_output/out.txt
    cd ../test
    """
    return shell_script, "Verifica que se pueda crear una carpeta en el filesystem vacio"

def test_mkdir_anidado(): 
    shell_script = """
    cd ../prueba
    ls >../test/received_output/out.txt
    mkdir carpeta 2>../test/received_output/err.txt
    ls >>../test/received_output/out.txt
    cd carpeta 2>>../test/received_output/err.txt
    mkdir carpeta2 2>>../../test/received_output/err.txt
    ls >>../../test/received_output/out.txt
    cd ../../test
    """
    return shell_script, "Verifica que se pueda crear una carpeta en el filesystem vacio"

def test_rmdir(): 
    shell_script = """
    cd ../prueba
    ls >../test/received_output/out.txt
    mkdir carpeta 2>../test/received_output/err.txt
    ls >>../test/received_output/out.txt
    rmdir carpeta 2>>../test/received_output/err.txt
    ls >> ../test/received_output/out.txt
    cd ../test
    """
    return shell_script, "Verifica que se pueda crear y eliminar una carpeta en el filesystem vacio"

def test_max_blocks(): 
    shell_script = """
    cd ../prueba
    mkdir carpeta1 2>../test/received_output/err.txt
    mkdir carpeta2 2>>../test/received_output/err.txt
    mkdir carpeta3 2>>../test/received_output/err.txt
    mkdir carpeta4 2>>../test/received_output/err.txt
    mkdir carpeta5 2>>../test/received_output/err.txt
    mkdir carpeta6 2>>../test/received_output/err.txt
    mkdir carpeta7 2>>../test/received_output/err.txt
    mkdir carpeta8 2>>../test/received_output/err.txt
    mkdir carpeta9 2>>../test/received_output/err.txt
    # 9 + root
        
    mkdir carpeta10 2>>../test/received_output/err.txt
    ls >> ../test/received_output/out.txt

    cd ../test
    """
    return shell_script, "Verifica que se pueda crear y eliminar una carpeta en el filesystem vacio"

def test_unlink(): 
    shell_script = """
    cd ../prueba
    echo "Hola Mundo" > n1.txt
    echo "Hola Mundo" > n2.txt
    echo "Hola Mundo" > n3.txt
    rm n2.txt
    ls 1>../test/received_output/out.txt 2>../test/received_output/err.txt
    cd ../test
    """
    return shell_script, "Verifica la eliminación archivos"


def test_unlink_muchos_archivos(): 
    shell_script = """
    cd ../prueba
    echo "Hola Mundo" > n1.txt
    echo "Hola Mundo" > n2.txt
    echo "Hola Mundo" > n3.txt
    echo "Hola Mundo" > n4.txt
    rm n4.txt
    rm n3.txt
    rm n2.txt
    rm n1.txt
    ls 1>../test/received_output/out.txt 2>../test/received_output/err.txt
    cd ../test
    """
    return shell_script, "Verifica la eliminación de varios archivos"

def test_ls_fisop():
    shell_script = """
    cd ../prueba
    touch file_1
    touch file_2
    ls 1>../test/received_output/out.txt 2>../test/received_output/err.txt
    cd ../test
    """
    return shell_script, "Verifica ls fisop"

def test_persistence_fisop():
    shell_script = """
    cd ../prueba
    echo "Hola Mundo" > p.txt
    ls 1>../test/received_output/out.txt 2>../test/received_output/err.txt
    cat p.txt 1>>../test/received_output/out.txt 2>>../test/received_output/err.txt
    cd ..
    sudo umount prueba
    sleep 1
    rm -rf prueba
    make clean
    sleep 1
    make
    mkdir prueba
    ./fisopfs prueba/
    cd prueba
    ls 1>>../test/received_output/out.txt 2>>../test/received_output/err.txt
    cat p.txt 1>>../test/received_output/out.txt 2>>../test/received_output/err.txt
    cd ../test
    """
    return shell_script, "Verifica persistencia del FS fisop"

def run_test(test_name, shell_script, description):
    # Ejecutar script
    retcode = run_shell_block(shell_script)
    if retcode != 0:
        print(f"{Colors.RED}{test_name} shell error con código {retcode}{Colors.RESET}")

    # Rutas para comparación
    expected_out = os.path.join('expected_output', test_name, 'out.txt')
    expected_err = os.path.join('expected_output', test_name, 'err.txt')
    received_out = os.path.join('received_output', 'out.txt')
    received_err = os.path.join('received_output', 'err.txt')

    # Comparar out.txt
    passed_out, diff_out = compare_files(expected_out, received_out)
    # Comparar err.txt
    passed_err, diff_err = compare_files(expected_err, received_err)

    # Manejo de tipos para diff_out y diff_err (pueden ser lista o string)
    diffs = []
    if isinstance(diff_out, list):
        diffs.extend(diff_out)
    else:
        diffs.append(str(diff_out))

    if isinstance(diff_err, list):
        diffs.extend(diff_err)
    else:
        diffs.append(str(diff_err))

    passed = passed_out and passed_err

    print_test_result(test_name, description, passed, diffs, received_out, received_err)

    # Post procesamiento: mover o borrar archivos
    if not passed:
        # Crear carpeta received_output/{test_name} si no existe
        dst_dir = os.path.join('received_output', test_name)
        os.makedirs(dst_dir, exist_ok=True)
        shutil.move(received_out, os.path.join(dst_dir, 'out.txt'))
        shutil.move(received_err, os.path.join(dst_dir, 'err.txt'))
    else:
        os.remove(received_out)
        os.remove(received_err)

def setup():
    shell_script = """
    mkdir received_output
    cd ..
    make
    mkdir prueba
    ./fisopfs prueba/
    """
    run_shell_block(shell_script)

def cleanup():
    shell_script = """
    cd ..
    umount prueba
    rm -rf prueba
    rm -rf persistence_file.fisopfs
    """
    run_shell_block(shell_script)
    return 


if __name__ == "__main__":
    tests = {
        'test_touch_fisop': test_touch_fisop,
        'test_echo_redir_fisop': test_echo_redir_fisop, 
        'test_echo_redir_a_archivo_existente_fisop': test_echo_redir_a_archivo_existente_fisop,
        'test_mkdir_fs_vacio': test_mkdir,
        'test_mkdir_anidado': test_mkdir_anidado,
        'test_unlink': test_unlink, 
        'test_unlink_muchos_archivos': test_unlink_muchos_archivos, 
        'test_rmdir': test_rmdir,
        'test_max_blocks': test_max_blocks,
        'test_ls_fisop': test_ls_fisop,
        'test_persistence_fisop': test_persistence_fisop,
    }
    
    for name, test_func in tests.items():
        setup()
        shell_script, description = test_func()
        run_test(name, shell_script, description)
        cleanup()