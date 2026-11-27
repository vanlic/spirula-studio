#pragma once

// What `spirula partition` prints and what the GUI's Partition panel says:
// splitting a reconstruction into parts that train separately, and merging
// the parts' models back into one. Paths, flag names and the words `core`
// and `ring` as identifiers stay as they are in every language.

#include "i18n/BeginCatalog.h"

namespace spirula {
namespace i18n {
namespace msg {
namespace partition {

// ===========================================================================
// spirula partition --help
// ===========================================================================

SS_MSG(tagline,
    EN("Split a reconstruction into parts that train separately, and merge the models back"),
    JA("再構成を別々に学習するパートに分割し、モデルをひとつに結合します"),
    ZH_HANS("把重建拆成可分别训练的分区，再把模型合并回来"),
    ZH_HANT("把重建拆成可分別訓練的分區，再把模型合併回來"),
    KO("재구성을 따로 학습할 파트로 나누고, 모델을 다시 하나로 병합합니다"),
    DE("Eine Rekonstruktion in getrennt trainierbare Teile zerlegen und die Modelle wieder zusammenführen"),
    FR("Découper une reconstruction en parties entraînées séparément, puis refusionner les modèles"),
    ES("Dividir una reconstrucción en partes que se entrenan por separado y fusionar los modelos"),
    PT("Dividir uma reconstrução em partes treinadas em separado e voltar a fundir os modelos"),
    IT("Dividere una ricostruzione in parti addestrate separatamente e riunire i modelli"),
    NL("Een reconstructie splitsen in apart getrainde delen en de modellen weer samenvoegen"),
    RU("Разбить реконструкцию на части, обучаемые раздельно, и объединить модели обратно"),
    TR("Bir yeniden oluşturmayı ayrı eğitilen parçalara bölmek ve modelleri geri birleştirmek"));

SS_MSG(usage_intro,
    EN("`split` partitions a COLMAP, Nerfstudio or Metashape dataset where cameras share the least.\n- Writes partition.json with visible regions and a ring of neighboring cameras.\n- Train each part with `spirula train --data <dataset> --partition <file> --partition-part <k>`.\n- `merge` combines the splats inside each part’s region."),
    JA("`split` は COLMAP / Nerfstudio / Metashape のデータセットを読み、カメラを共有の最も少ない所で切り、各パートにそのカメラが見る領域と、それを見ている外側のカメラの輪を与えて partition.json を書き出します。\n各パートは `spirula train --data <dataset> --partition <file> --partition-part <k>` で学習し、`merge` が各モデルのスプラットを自分の領域内だけ残してひとつのファイルにまとめます。"),
    ZH_HANS("`split` 读取 COLMAP、Nerfstudio 或 Metashape 数据集，在相机共视最少处切开，给每个分区分配其相机所见的区域和同样看到该区域的外围相机环，写出 partition.json。\n用 `spirula train --data <dataset> --partition <file> --partition-part <k>` 训练各分区，再用 `merge` 只保留各模型在自身区域内的泼溅并写成一个文件。"),
    ZH_HANT("`split` 讀取 COLMAP、Nerfstudio 或 Metashape 資料集，在相機共視最少處切開，給每個分區分配其相機所見的區域和同樣看到該區域的外圍相機環，寫出 partition.json。\n用 `spirula train --data <dataset> --partition <file> --partition-part <k>` 訓練各分區，再用 `merge` 只保留各模型在自身區域內的潑濺並寫成一個檔案。"),
    KO("`split`은 COLMAP, Nerfstudio 또는 Metashape 데이터셋을 읽어 카메라를 가장 적게 공유하는 곳에서 잘라, 각 파트에 그 카메라가 보는 영역과 그 영역을 함께 보는 바깥 카메라의 고리를 주고 partition.json을 씁니다.\n각 파트는 `spirula train --data <dataset> --partition <file> --partition-part <k>`로 학습하고, `merge`가 각 모델의 스플랫을 자기 영역 안의 것만 남겨 한 파일로 씁니다."),
    DE("`split` liest einen COLMAP-, Nerfstudio- oder Metashape-Datensatz, schneidet die Kameras dort, wo sie am wenigsten gemeinsam sehen, gibt jedem Teil den Bereich, den seine Kameras sehen, und einen Ring äußerer Kameras, die ihn mitsehen, und schreibt partition.json.\n- Jeder Teil wird mit `spirula train --data <dataset> --partition <file> --partition-part <k>` trainiert;\n- `merge` behält von jedem Modell die Splats im eigenen Bereich und schreibt sie als eine Datei."),
    FR("`split` lit un jeu de données COLMAP, Nerfstudio ou Metashape, coupe les caméras là où elles partagent le moins, donne à chaque partie la région que voient ses caméras et un anneau de caméras extérieures qui la voient aussi, et écrit partition.json.\nEntraînez chaque partie avec `spirula train --data <dataset> --partition <file> --partition-part <k>`, puis `merge` garde de chaque modèle les splats de sa propre région et les écrit en un seul fichier."),
    ES("`split` lee un conjunto COLMAP, Nerfstudio o Metashape, corta las cámaras donde menos comparten, da a cada parte la región que ven sus cámaras y un anillo de cámaras externas que también la ven, y escribe partition.json.\n- Entrena cada parte con `spirula train --data <dataset> --partition <file> --partition-part <k>`;\n- después `merge` conserva de cada modelo los splats dentro de su región y los escribe en un solo archivo."),
    PT("`split` lê um conjunto COLMAP, Nerfstudio ou Metashape, corta as câmaras onde menos partilham, dá a cada parte a região que as suas câmaras veem e um anel de câmaras exteriores que também a veem, e escreve partition.json.\n- Treine cada parte com `spirula train --data <dataset> --partition <file> --partition-part <k>`;\n- depois `merge` guarda de cada modelo os splats dentro da sua região e escreve-os num só ficheiro."),
    IT("`split` legge un insieme COLMAP, Nerfstudio o Metashape, taglia le fotocamere dove condividono meno, dà a ogni parte la regione che le sue fotocamere vedono e un anello di fotocamere esterne che la vedono anche loro, e scrive partition.json. Addestra ogni parte con `spirula train --data <dataset> --partition <file> --partition-part <k>`;\npoi `merge` tiene di ogni modello gli splat dentro la propria regione e li scrive in un solo file."),
    NL("`split` leest een COLMAP-, Nerfstudio- of Metashape-dataset, snijdt de camera's waar ze het minst delen, geeft elk deel het gebied dat zijn camera's zien plus een ring buitencamera's die het ook zien, en schrijft partition.json.\n- Train elk deel met `spirula train --data <dataset> --partition <file> --partition-part <k>`;\n- daarna houdt `merge` van elk model de splats binnen het eigen gebied en schrijft ze als één bestand."),
    RU("`split` читает набор COLMAP, Nerfstudio или Metashape, режет камеры там, где они меньше всего пересекаются, даёт каждой части область, которую видят её камеры, и кольцо внешних камер, которые тоже её видят, и пишет partition.json.\nКаждую часть обучают `spirula train --data <dataset> --partition <file> --partition-part <k>`, затем `merge` оставляет от каждой модели сплаты внутри её области и записывает их одним файлом."),
    TR("`split` bir COLMAP, Nerfstudio ya da Metashape veri kümesini okur, kameraları en az paylaştıkları yerden keser, her parçaya kameralarının gördüğü bölgeyi ve onu da gören dış kameralardan bir halka verir ve partition.json yazar.\n- Her parçayı `spirula train --data <dataset> --partition <file> --partition-part <k>` ile eğitin;\n- sonra `merge` her modelin kendi bölgesindeki splatlarını tutup tek dosya yazar."));

SS_MSG(head_options,
    EN("Options"), JA("オプション"), ZH_HANS("选项"), ZH_HANT("選項"),
    KO("옵션"), DE("Optionen"), FR("Options"), ES("Opciones"), PT("Opções"),
    IT("Opzioni"), NL("Opties"), RU("Параметры"), TR("Seçenekler"));

SS_MSG(head_split,
    EN("split options"), JA("split のオプション"), ZH_HANS("split 选项"),
    ZH_HANT("split 選項"), KO("split 옵션"), DE("Optionen für split"),
    FR("options de split"), ES("opciones de split"), PT("opções de split"),
    IT("opzioni di split"), NL("opties van split"), RU("параметры split"),
    TR("split seçenekleri"));

SS_MSG(head_merge,
    EN("merge options"), JA("merge のオプション"), ZH_HANS("merge 选项"),
    ZH_HANT("merge 選項"), KO("merge 옵션"), DE("Optionen für merge"),
    FR("options de merge"), ES("opciones de merge"), PT("opções de merge"),
    IT("opzioni di merge"), NL("opties van merge"), RU("параметры merge"),
    TR("merge seçenekleri"));

SS_MSG(opt_parts,
    EN("Number of parts; 0 uses --max-images as a camera target. Default: 0."),
    JA("パート数。0 は --max-images をカメラ数の目安に使用。既定: 0。"),
    ZH_HANS("分区数；0 使用 --max-images 作为相机数量目标。默认：0。"),
    ZH_HANT("分區數；0 使用 --max-images 作為相機數量目標。預設：0。"),
    KO("파트 수. 0은 --max-images를 카메라 수 목표로 사용. 기본값: 0."),
    DE("Anzahl der Teile; 0 nutzt --max-images als Kameraziel. Standard: 0."),
    FR("Nombre de parties ; 0 vise --max-images caméras. Défaut : 0."),
    ES("Número de partes; 0 usa --max-images como objetivo de cámaras. Valor inicial: 0."),
    PT("Número de partes; 0 usa --max-images como meta de câmaras. Padrão: 0."),
    IT("Numero di parti; 0 usa --max-images come obiettivo di fotocamere. Default: 0."),
    NL("Aantal delen; 0 gebruikt --max-images als cameradoel. Standaard: 0."),
    RU("Число частей; 0 задаёт цель по камерам через --max-images. По умолчанию: 0."),
    TR("Parça sayısı; 0 kamera hedefi olarak --max-images kullanır. Varsayılan: 0."));

SS_MSG(opt_max_images,
    EN("Target cameras per part when --parts is 0. Overlap can exceed it. Default: 2000."),
    JA("--parts が 0 のときのカメラ数の目安。重複で超える場合があります。既定: 2000。"),
    ZH_HANS("--parts 为 0 时每区的相机数量目标。重叠可超出此值。默认：2000。"),
    ZH_HANT("--parts 為 0 時每區的相機數量目標。重疊可超出此值。預設：2000。"),
    KO("--parts가 0일 때 파트별 카메라 수 목표. 중첩으로 초과할 수 있음. 기본값: 2000."),
    DE("Kameraziel je Teil bei --parts 0. Überlappung kann es überschreiten. Standard: 2000."),
    FR("Cible de caméras par partie si --parts vaut 0. Le recouvrement peut la dépasser. Défaut : 2000."),
    ES("Objetivo de cámaras por parte si --parts es 0. El solapamiento puede superarlo. Valor inicial: 2000."),
    PT("Meta de câmaras por parte se --parts for 0. A sobreposição pode excedê-la. Padrão: 2000."),
    IT("Obiettivo di fotocamere per parte con --parts 0. La sovrapposizione può superarlo. Default: 2000."),
    NL("Cameradoel per deel bij --parts 0. Overlap kan dit overschrijden. Standaard: 2000."),
    RU("Цель по числу камер при --parts 0. Перекрытие может её превысить. По умолчанию: 2000."),
    TR("--parts 0 iken parça başına kamera hedefi. Örtüşme bunu aşabilir. Varsayılan: 2000."));

SS_MSG(opt_ring,
    EN("A camera outside a part joins its ring when at least this share of the "
       "points it sees belongs to the part. Default: 0.1"),
    JA("パート外のカメラは、見ている点のうちこの割合以上がそのパートに属するとき"
       "リングに加わります。既定: 0.1"),
    ZH_HANS("分区外的相机所见点中属于该分区的比例达到此值时，加入其外环。默认：0.1"),
    ZH_HANT("分區外的相機所見點中屬於該分區的比例達到此值時，加入其外環。預設：0.1"),
    KO("파트 밖의 카메라는 자신이 보는 점 중 이 비율 이상이 그 파트에 속할 때 고리에 "
       "들어갑니다. 기본값: 0.1"),
    DE("Eine Kamera außerhalb eines Teils tritt seinem Ring bei, wenn mindestens "
       "dieser Anteil der von ihr gesehenen Punkte zum Teil gehört. Standard: 0.1"),
    FR("Une caméra hors d'une partie rejoint son anneau quand au moins cette part "
       "des points qu'elle voit appartient à la partie. Défaut : 0.1"),
    ES("Una cámara fuera de una parte entra en su anillo cuando al menos esta "
       "fracción de los puntos que ve pertenece a la parte. Predeterminado: 0.1"),
    PT("Uma câmara fora de uma parte entra no seu anel quando pelo menos esta "
       "fração dos pontos que vê pertence à parte. Predefinição: 0.1"),
    IT("Una fotocamera fuori da una parte entra nel suo anello quando almeno "
       "questa quota dei punti che vede appartiene alla parte. Predefinito: 0.1"),
    NL("Een camera buiten een deel komt in zijn ring als minstens dit aandeel van "
       "de punten die ze ziet bij het deel hoort. Standaard: 0.1"),
    RU("Камера вне части входит в её кольцо, если хотя бы такая доля видимых ею "
       "точек принадлежит части. По умолчанию: 0.1"),
    TR("Bir parçanın dışındaki kamera, gördüğü noktaların en az bu payı parçaya "
       "aitse halkasına katılır. Varsayılan: 0.1"));

SS_MSG(opt_ring_min,
    EN("...and at least this many of them. Default: 20"),
    JA("…かつ点の数がこれ以上のとき。既定: 20"),
    ZH_HANS("……且这样的点至少有这么多。默认：20"),
    ZH_HANT("……且這樣的點至少有這麼多。預設：20"),
    KO("...그리고 그런 점이 최소 이만큼일 때. 기본값: 20"),
    DE("... und mindestens so viele davon. Standard: 20"),
    FR("... et au moins ce nombre d'entre eux. Défaut : 20"),
    ES("... y al menos este número de ellos. Predeterminado: 20"),
    PT("... e pelo menos este número deles. Predefinição: 20"),
    IT("... e almeno questo numero di essi. Predefinito: 20"),
    NL("... en minstens zoveel daarvan. Standaard: 20"),
    RU("... и не меньше такого их числа. По умолчанию: 20"),
    TR("... ve bunlardan en az bu kadarı. Varsayılan: 20"));

SS_MSG(opt_max_seeds,
    EN("Maximum sampled points for graph ownership fields. Default: 1000000."),
    JA("グラフ所有領域の点サンプル上限。既定: 1000000。"),
    ZH_HANS("图归属场的最大采样点数。默认：1000000。"),
    ZH_HANT("圖歸屬場的最大採樣點數。預設：1000000。"),
    KO("그래프 소유 영역의 최대 샘플 점 수. 기본값: 1000000."),
    DE("Maximale Punktzahl für Graph-Zugehörigkeitsfelder. Standard: 1000000."),
    FR("Maximum de points pour les champs du graphe. Défaut : 1000000."),
    ES("Máximo de puntos del campo de propiedad del grafo. Predeterminado: 1000000."),
    PT("Máximo de pontos do campo de propriedade do grafo. Padrão: 1000000."),
    IT("Massimo di punti per i campi di proprietà del grafo. Default: 1000000."),
    NL("Maximaal punten voor eigendomsvelden van de graaf. Standaard: 1000000."),
    RU("Предел точек для полей принадлежности графа. По умолчанию: 1000000."),
    TR("Grafik sahiplik alanları için azami örnek nokta. Varsayılan: 1000000."));

SS_MSG(opt_method,
    EN("- viewgraph (default): group covisible cameras.\n- graph: group cameras and visible points.\n- voronoi: compact cells around camera clusters.\n- lcc: split by visible pixel budget."),
    JA("- viewgraph（既定）: 共視カメラをまとめます。\n- graph: カメラと可視点をまとめます。\n- voronoi: カメラ群の周りを凸領域に分割します。\n- lcc: 可視画素数の上限で分割。"),
    ZH_HANS("- viewgraph（默认）：按共视关系分组相机。\n- graph：将相机和可见点共同分组。\n- voronoi：围绕相机簇划分紧凑区域。\n- lcc：按可见像素预算切分。"),
    ZH_HANT("- viewgraph（預設）：按共視關係分組相機。\n- graph：將相機和可見點共同分組。\n- voronoi：圍繞相機群劃分緊湊區域。\n- lcc：按可見像素預算切分。"),
    KO("- viewgraph(기본): 공가시 카메라를 묶습니다.\n- graph: 카메라와 가시점을 함께 묶습니다.\n- voronoi: 카메라 군집 주위의 조밀한 영역입니다.\n- lcc: 가시 픽셀 예산으로 분할."),
    DE("- viewgraph (Standard): gemeinsam sehende Kameras.\n- graph: Kameras und sichtbare Punkte gruppieren.\n- voronoi: kompakte Zellen um Kameragruppen.\n- lcc: nach sichtbarem Pixelbudget teilen."),
    FR("- viewgraph (défaut) : caméras à vues communes.\n- graph : groupe caméras et points visibles.\n- voronoi : cellules compactes autour des caméras.\n- lcc : découpe selon le budget de pixels visibles."),
    ES("- viewgraph (predeterminado): cámaras covisibles.\n- graph: agrupa cámaras y puntos visibles.\n- voronoi: celdas compactas alrededor de las cámaras.\n- lcc: divide por presupuesto de píxeles visibles."),
    PT("- viewgraph (padrão): câmeras com vistas comuns.\n- graph: agrupa câmeras e pontos visíveis.\n- voronoi: células compactas em torno das câmeras.\n- lcc: divide pelo orçamento de píxeis visíveis."),
    IT("- viewgraph (default): camere con viste comuni.\n- graph: raggruppa camere e punti visibili.\n- voronoi: celle compatte attorno ai gruppi di camere.\n- lcc: divide per budget di pixel visibili."),
    NL("- viewgraph (standaard): camera's met gedeeld zicht.\n- graph: groepeert camera's en zichtbare punten.\n- voronoi: compacte cellen rond cameragroepen.\n- lcc: deelt op budget voor zichtbare pixels."),
    RU("- viewgraph (по умолчанию): камеры с общим обзором.\n- graph: группы камер и видимых точек.\n- voronoi: компактные ячейки вокруг групп камер.\n- lcc: деление по бюджету видимых пикселей."),
    TR("- viewgraph (varsayılan): ortak görüşlü kameralar.\n- graph: kameraları ve görünen noktaları gruplar.\n- voronoi: kamera grupları çevresinde kompakt hücreler.\n- lcc: görünür piksel bütçesine göre böler."));

SS_MSG(meth_lcc,
    EN("Visible pixels (LCC)"),
    JA("可視画素（LCC）"),
    ZH_HANS("可见像素（LCC）"),
    ZH_HANT("可見像素（LCC）"),
    KO("가시 픽셀(LCC)"),
    DE("Sichtbare Pixel (LCC)"),
    FR("Pixels visibles (LCC)"),
    ES("Píxeles visibles (LCC)"),
    PT("Píxeis visíveis (LCC)"),
    IT("Pixel visibili (LCC)"),
    NL("Zichtbare pixels (LCC)"),
    RU("Видимые пиксели (LCC)"),
    TR("Görünür pikseller (LCC)"));

SS_MSG(lbl_gpixels,
    EN("GPixels per block"),
    JA("ブロックの GPixels"),
    ZH_HANS("每块 GPixels"),
    ZH_HANT("每塊 GPixels"),
    KO("블록당 GPixels"),
    DE("GPixels pro Block"),
    FR("GPixels par bloc"),
    ES("GPixels por bloque"),
    PT("GPixels por bloco"),
    IT("GPixels per blocco"),
    NL("GPixels per blok"),
    RU("GPixels на блок"),
    TR("Blok başına GPixels"));

SS_MSG(lbl_lcc_lidar,
    EN("LiDAR point cloud"), JA("LiDAR 点群"), ZH_HANS("激光雷达点云"),
    ZH_HANT("雷射雷達點雲"), KO("라이다 포인트 클라우드"), DE("LiDAR-Punktwolke"),
    FR("Nuage de points LiDAR"), ES("Nube de puntos LiDAR"), PT("Nuvem de pontos LiDAR"),
    IT("Nuvola di punti LiDAR"), NL("LiDAR-puntenwolk"), RU("Облако точек LiDAR"),
    TR("LiDAR nokta bulutu"));

SS_MSG(preview_sfm_cloud,
    EN("SfM point cloud"), JA("SfM 点群"), ZH_HANS("SfM 点云"),
    ZH_HANT("SfM 點雲"), KO("SfM 포인트 클라우드"), DE("SfM-Punktwolke"),
    FR("Nuage de points SfM"), ES("Nube de puntos SfM"), PT("Nuvem de pontos SfM"),
    IT("Nuvola di punti SfM"), NL("SfM-puntenwolk"), RU("Облако точек SfM"),
    TR("SfM nokta bulutu"));

SS_MSG(status_reading_lidar,
    EN("Loading LiDAR preview…"), JA("LiDAR のプレビューを読み込み中…"),
    ZH_HANS("正在加载激光雷达预览…"), ZH_HANT("正在載入雷射雷達預覽…"),
    KO("라이다 미리보기 불러오는 중…"), DE("LiDAR-Vorschau wird geladen …"),
    FR("Chargement de l’aperçu LiDAR…"), ES("Cargando vista previa LiDAR…"),
    PT("Carregando prévia LiDAR…"), IT("Caricamento anteprima LiDAR…"),
    NL("LiDAR-voorbeeld laden…"), RU("Загрузка предпросмотра LiDAR…"),
    TR("LiDAR önizlemesi yükleniyor…"));

SS_MSG(err_empty_lidar_preview,
    EN("The LiDAR file has no points to preview."), JA("この LiDAR ファイルには表示できる点がありません。"),
    ZH_HANS("该激光雷达文件没有可预览的点。"), ZH_HANT("此雷射雷達檔案沒有可預覽的點。"),
    KO("라이다 파일에 미리 볼 점이 없습니다."), DE("Die LiDAR-Datei enthält keine Punkte für die Vorschau."),
    FR("Ce fichier LiDAR ne contient aucun point à afficher."),
    ES("El archivo LiDAR no contiene puntos que mostrar."),
    PT("O arquivo LiDAR não contém pontos para exibir."),
    IT("Il file LiDAR non contiene punti da mostrare."),
    NL("Het LiDAR-bestand bevat geen punten om weer te geven."),
    RU("В файле LiDAR нет точек для просмотра."), TR("LiDAR dosyasında önizlenecek nokta yok."));

SS_MSG(opt_lcc_lidar,
    EN("Choose a scan aligned to the dataset. Leave blank to use dataset points for partitioning."),
    JA("データセットに整列したスキャンを選びます。空欄ならデータセットの点で分割します。"),
    ZH_HANS("选择已对齐到数据集的点云。留空时用数据集点云分区。"),
    ZH_HANT("選擇已對齊到資料集的點雲。留空時用資料集點雲分區。"),
    KO("데이터셋에 정렬된 스캔을 선택합니다. 비워 두면 데이터셋의 점으로 분할합니다."),
    DE("Scan im Datensatz-Koordinatensystem wählen. Leer nutzt Datensatzpunkte."),
    FR("Choisir un scan aligné sur le jeu de données. Vide, utilise les points du jeu."),
    ES("Elige un escaneo alineado con el conjunto. Si se deja vacío, se usan sus puntos."),
    PT("Escolha uma varredura alinhada ao conjunto. Em branco, usa os pontos do conjunto."),
    IT("Scegli una scansione allineata al dataset. Vuoto usa i punti del dataset."),
    NL("Kies een scan uitgelijnd op de dataset. Leeg gebruikt datasetpunten."),
    RU("Выберите скан в системе координат набора. Пусто — использовать точки набора."),
    TR("Veri kümesine hizalı bir tarama seçin. Boşsa veri kümesinin noktaları kullanılır."));

SS_MSG(lbl_lcc_lidar_sample,
    EN("LiDAR sample fraction"), JA("LiDAR 点の抽出率"), ZH_HANS("激光点采样比例"),
    ZH_HANT("雷射點取樣比例"), KO("라이다 샘플링 비율"), DE("LiDAR-Stichprobenanteil"),
    FR("Fraction LiDAR échantillonnée"), ES("Fracción de muestreo LiDAR"),
    PT("Fração de amostragem LiDAR"), IT("Frazione di campionamento LiDAR"),
    NL("LiDAR-steekproeffractie"), RU("Доля точек LiDAR"), TR("LiDAR örnekleme oranı"));

SS_MSG(opt_lcc_lidar_sample,
    EN("0 uses 20% for PLY and 3% for other scans. 1 uses the full scan up to the projection limit."),
    JA("0 は PLY を 20%、他を 3% 抽出します。1 は投影上限まで全点を使います。"),
    ZH_HANS("0 为自动，PLY 取 20%，其他格式取 3%。1 使用全量，受投影点数上限限制。"),
    ZH_HANT("0 為自動，PLY 取 20%，其他格式取 3%。1 使用全量，受投影點數上限限制。"),
    KO("0은 PLY 20%, 그 외 3%입니다. 1은 투영 한도까지 모든 점을 사용합니다."),
    DE("0 nutzt 20 % bei PLY und 3 % sonst. 1 nutzt alle Punkte bis zur Projektionsgrenze."),
    FR("0 prend 20 % des PLY et 3 % des autres scans. 1 prend tout jusqu’à la limite de projection."),
    ES("0 toma el 20 % de PLY y el 3 % de los demás. 1 toma todo hasta el límite de proyección."),
    PT("0 usa 20% de PLY e 3% dos demais. 1 usa todos os pontos até o limite de projeção."),
    IT("0 usa il 20% per PLY e il 3% per gli altri. 1 usa tutto fino al limite di proiezione."),
    NL("0 gebruikt 20% voor PLY en 3% voor andere scans. 1 gebruikt alles tot de projectielimiet."),
    RU("0 берёт 20% из PLY и 3% из других форматов. 1 берёт все точки до лимита проекции."),
    TR("0, PLY için %20, diğerleri için %3 alır. 1, izdüşüm sınırına kadar tüm noktaları alır."));

SS_MSG(lbl_lcc_voxel,
    EN("Voxel size"),
    JA("ボクセルサイズ"),
    ZH_HANS("体素尺寸"),
    ZH_HANT("體素尺寸"),
    KO("복셀 크기"),
    DE("Voxelgröße"),
    FR("Taille du voxel"),
    ES("Tamaño de vóxel"),
    PT("Tamanho do voxel"),
    IT("Dimensione voxel"),
    NL("Voxelgrootte"),
    RU("Размер вокселя"),
    TR("Voksel boyutu"));

SS_MSG(lbl_lcc_overlap,
    EN("Max overlap"),
    JA("最大重複幅"),
    ZH_HANS("最大外扩宽度"),
    ZH_HANT("最大外擴寬度"),
    KO("최대 중첩 폭"),
    DE("Max. Überlappung"),
    FR("Recouvrement max."),
    ES("Solapamiento máx."),
    PT("Sobreposição máx."),
    IT("Sovrapposizione max"),
    NL("Max. overlap"),
    RU("Макс. перекрытие"),
    TR("Azami örtüşme"));

SS_MSG(lbl_lcc_sky_ring,
    EN("Sky ring images"), JA("空に基づく外周画像"), ZH_HANS("按天空占比补外环"), ZH_HANT("按天空佔比補外環"),
    KO("하늘 기반 링 이미지"), DE("Ringbilder nach Himmel"), FR("Vues d’anneau par ciel"),
    ES("Vistas del anillo por cielo"), PT("Vistas do anel pelo céu"), IT("Viste anello per cielo"),
    NL("Ringbeelden op basis van lucht"), RU("Кадры кольца по небу"), TR("Göğe göre halka görüntüleri"));
SS_MSG(opt_lcc_sky_ring,
    EN("Add ring images by the share of valid pixels with an open upward ray over the part."),
    JA("区画の上を通る遮蔽のない上向きの有効画素の割合で外周画像を追加します。"),
    ZH_HANS("按朝上、未被点云遮挡且经过分块上方的有效像素比例补充外环图像。"),
    ZH_HANT("按朝上、未被點雲遮擋且經過分塊上方的有效像素比例補充外環影像。"),
    KO("파트 위로 향하는 가리지 않은 유효 픽셀의 비율로 링 이미지를 추가합니다."),
    DE("Fügt Ringbilder nach dem Anteil freier Aufwärtsstrahlen über dem Block hinzu."),
    FR("Ajoute des vues selon la part des rayons libres dirigés au-dessus du bloc."),
    ES("Añade vistas según la proporción de rayos libres que pasan por encima del bloque."),
    PT("Adiciona vistas pela proporção de raios livres que passam acima do bloco."),
    IT("Aggiunge viste in base ai raggi liberi diretti sopra il blocco."),
    NL("Voegt ringbeelden toe op basis van vrije opwaartse stralen boven het blok."),
    RU("Добавляет кадры по доле открытых лучей, направленных вверх над блоком."),
    TR("Bloğun üstüne uzanan açık ışınların oranına göre halka görüntüsü ekler."));
SS_MSG(lbl_lcc_sky_angle,
    EN("Fisheye angle limit (°)"), JA("魚眼の角度上限 (°)"), ZH_HANS("鱼眼夹角上限（°）"), ZH_HANT("魚眼夾角上限（°）"),
    KO("어안 각도 한계 (°)"), DE("Fischaugen-Winkelgrenze (°)"), FR("Angle fisheye max. (°)"),
    ES("Ángulo ojo de pez máx. (°)"), PT("Ângulo olho de peixe máx. (°)"), IT("Angolo fisheye max (°)"),
    NL("Max. fisheye-hoek (°)"), RU("Макс. угол фишая (°)"), TR("Balıkgözü açı sınırı (°)"));
SS_MSG(opt_lcc_sky_angle,
    EN("Skip a part when the angle to its nearest boundary from the fisheye axis exceeds this limit."),
    JA("魚眼の光軸から区画の最も近い境界への角度が上限を超えると省略します。"),
    ZH_HANS("鱼眼光轴和指向分块最近边界的方向夹角超过此值时，跳过天空计算。"),
    ZH_HANT("魚眼光軸和指向分塊最近邊界的方向夾角超過此值時，跳過天空計算。"),
    KO("어안 광축과 가장 가까운 파트 경계까지의 방향이 한계를 넘으면 건너뜁니다."),
    DE("Überspringt Blöcke, deren nächste Grenze außerhalb dieses Winkels zur Fischaugenachse liegt."),
    FR("Ignore les blocs dont le bord le plus proche dépasse cet angle par rapport à l’axe fisheye."),
    ES("Omite el bloque si el borde más cercano supera este ángulo respecto al eje del ojo de pez."),
    PT("Ignora blocos cuja borda mais próxima excede este ângulo em relação ao eixo olho de peixe."),
    IT("Salta il blocco se il bordo più vicino supera questo angolo rispetto all’asse fisheye."),
    NL("Slaat een blok over als de dichtstbijzijnde rand te ver van de fisheye-as afwijkt."),
    RU("Пропускает блок, если направление к ближайшей границе слишком отклонено от оси фишая."),
    TR("En yakın blok sınırının balıkgözü ekseniyle açısı bu sınırı aşarsa blok atlanır."));
SS_MSG(lbl_lcc_sky_percent,
    EN("Sky threshold (%)"), JA("空のしきい値 (%)"), ZH_HANS("天空占比阈值（%）"), ZH_HANT("天空佔比門檻（%）"),
    KO("하늘 비율 기준 (%)"), DE("Himmelsschwelle (%)"), FR("Seuil de ciel (%)"), ES("Umbral de cielo (%)"),
    PT("Limiar de céu (%)"), IT("Soglia cielo (%)"), NL("Luchtdrempel (%)"), RU("Порог неба (%)"),
    TR("Gök eşiği (%)"));

SS_MSG(lbl_lcc_robust_bounds,
    EN("Robust bounds"), JA("外れ値に強い境界"),
    ZH_HANS("稳健边界"), ZH_HANT("穩健邊界"),
    KO("강건한 경계"), DE("Robuste Grenzen"),
    FR("Bornes robustes"), ES("Límites robustos"),
    PT("Limites robustos"), IT("Limiti robusti"),
    NL("Robuuste grenzen"), RU("Устойчивые границы"), TR("Aykırı değerlere dayanıklı sınırlar"));

SS_MSG(opt_lcc_robust_bounds,
    EN("Exclude points outside padded percentile bounds before partitioning."),
    JA("分割前に、余白付きパーセンタイル範囲の外にある点を除外します。"),
    ZH_HANS("分区前排除带余量的百分位边界之外的点。"),
    ZH_HANT("分區前排除帶餘量的百分位邊界之外的點。"),
    KO("분할 전에 여유를 둔 백분위수 범위 밖의 점을 제외합니다."),
    DE("Punkte außerhalb erweiterter Perzentilgrenzen vor dem Teilen ausschließen."),
    FR("Exclut les points hors des bornes par centiles élargies avant la partition."),
    ES("Excluye los puntos fuera de los límites percentiles ampliados antes de dividir."),
    PT("Exclui pontos fora dos limites de percentis ampliados antes da divisão."),
    IT("Esclude i punti fuori dai limiti percentili ampliati prima della divisione."),
    NL("Sluit vóór het verdelen punten buiten de verruimde percentielgrenzen uit."),
    RU("Исключает точки вне расширенных процентильных границ перед разбиением."),
    TR("Bölmeden önce genişletilmiş yüzdelik sınırları dışındaki noktaları eler."));

SS_MSG(lbl_lcc_min_track_length,
    EN("Min. track length"), JA("最小トラック長"),
    ZH_HANS("最少 Track 观测数"), ZH_HANT("最少 Track 觀測數"),
    KO("최소 트랙 길이"), DE("Min. Tracklänge"),
    FR("Longueur min. de piste"), ES("Longitud mín. de pista"),
    PT("Comp. mín. da trilha"), IT("Lunghezza min. traccia"),
    NL("Min. spoorlengte"), RU("Мин. длина трека"), TR("En az iz uzunluğu"));

SS_MSG(opt_lcc_min_track_length,
    EN("Minimum COLMAP image observations per seed point. 0 keeps all points."),
    JA("種点ごとの COLMAP 画像観測数の下限。0 で全点を保持します。"),
    ZH_HANS("每个种子点所需的最少 COLMAP 图像观测数。0 保留所有点。"),
    ZH_HANT("每個種子點所需的最少 COLMAP 影像觀測數。0 保留所有點。"),
    KO("시드 점당 최소 COLMAP 이미지 관측 수입니다. 0은 모든 점을 유지합니다."),
    DE("Min. COLMAP-Bildbeobachtungen je Startpunkt. 0 behält alle Punkte."),
    FR("Nombre min. d'observations COLMAP par point initial. 0 garde tous les points."),
    ES("Mín. de observaciones COLMAP por punto inicial. 0 conserva todos los puntos."),
    PT("Mín. de observações COLMAP por ponto inicial. 0 mantém todos os pontos."),
    IT("Min. osservazioni COLMAP per punto iniziale. 0 conserva tutti i punti."),
    NL("Min. COLMAP-beeldwaarnemingen per startpunt. 0 behoudt alle punten."),
    RU("Мин. число наблюдений COLMAP на исходную точку. 0 сохраняет все точки."),
    TR("Başlangıç noktası başına en az COLMAP gözlemi. 0 tüm noktaları tutar."));

SS_MSG(opt_gpixels,
    EN("LCC visible pixel budget per block in GPixels. Default: 16."),
    JA("LCC のブロックごとの可視画素上限（GPixels）。既定値: 16。"),
    ZH_HANS("LCC 每块可见像素预算，单位 GPixels。默认 16。"),
    ZH_HANT("LCC 每塊可見像素預算，單位 GPixels。預設 16。"),
    KO("LCC 블록당 가시 픽셀 예산(GPixels). 기본값: 16."),
    DE("LCC-Budget sichtbarer Pixel je Block in GPixels. Standard: 16."),
    FR("Budget LCC de pixels visibles par bloc en GPixels. Défaut : 16."),
    ES("Presupuesto LCC de píxeles visibles por bloque en GPixels. Valor inicial: 16."),
    PT("Orçamento LCC de píxeis visíveis por bloco em GPixels. Padrão: 16."),
    IT("Budget LCC di pixel visibili per blocco in GPixels. Default: 16."),
    NL("LCC-budget voor zichtbare pixels per blok in GPixels. Standaard: 16."),
    RU("Бюджет видимых пикселей LCC на блок в GPixels. По умолчанию: 16."),
    TR("LCC blok başına görünür piksel bütçesi (GPixels). Varsayılan: 16."));

SS_MSG(opt_lcc_voxel,
    EN("LCC voxel size in dataset units. 0 selects it automatically."),
    JA("LCC のボクセル幅（データセット単位）。0 は自動選択。"),
    ZH_HANS("LCC 体素尺寸，使用数据集单位。0 为自动。"),
    ZH_HANT("LCC 體素尺寸，使用資料集單位。0 為自動。"),
    KO("LCC 복셀 크기(데이터셋 단위). 0은 자동."),
    DE("LCC-Voxelgröße in Datensatzeinheiten. 0 wählt sie automatisch."),
    FR("Taille de voxel LCC en unités du jeu de données. 0 pour le mode auto."),
    ES("Tamaño de vóxel LCC en unidades del conjunto. 0 lo elige automáticamente."),
    PT("Tamanho do voxel LCC nas unidades dos dados. 0 escolhe automaticamente."),
    IT("Dimensione voxel LCC in unità del dataset. 0 per la scelta automatica."),
    NL("LCC-voxelgrootte in dataseteenheden. 0 kiest automatisch."),
    RU("Размер вокселя LCC в единицах набора. 0 выбирает автоматически."),
    TR("Veri kümesi biriminde LCC voksel boyutu. 0 otomatik seçer."));

SS_MSG(opt_lcc_overlap,
    EN("Max LCC padding per side in dataset units. 0 uses adaptive padding."),
    JA("LCC の片側の最大余白（データセット単位）。0 は自動調整。"),
    ZH_HANS("LCC 每侧最大外扩宽度，使用数据集单位。0 为自适应。"),
    ZH_HANT("LCC 每側最大外擴寬度，使用資料集單位。0 為自適應。"),
    KO("한쪽의 최대 LCC 여백(데이터셋 단위). 0은 자동 조절."),
    DE("Max. LCC-Rand pro Seite in Datensatzeinheiten. 0 passt ihn an."),
    FR("Marge LCC max. par côté en unités du jeu de données. 0 adapte la marge."),
    ES("Margen LCC máximo por lado en unidades del conjunto. 0 lo adapta."),
    PT("Margem LCC máxima por lado nas unidades dos dados. 0 adapta a margem."),
    IT("Margine LCC max per lato in unità del dataset. 0 adatta il margine."),
    NL("Max. LCC-marge per zijde in dataseteenheden. 0 past deze aan."),
    RU("Макс. отступ LCC с каждой стороны в единицах набора. 0 подбирает отступ."),
    TR("Kenar başına azami LCC payı, veri kümesi biriminde. 0 uyarlanır."));

SS_MSG(opt_source,
    EN("- Where covisibility comes from.\n- auto takes the model's own tracks, else projects the seed cloud into every frame, else uses camera positions alone.\n- Default: auto"),
    JA("- 共視性の出どころ。\n- auto はモデル自身のトラックを使い、なければ初期点群を全フレームに投影し、それもなければカメラ位置だけを使います。\n- 既定: auto"),
    ZH_HANS("共视关系的来源。auto 优先用模型自带的轨迹，否则把种子点云投影到每一帧，再否则只用相机位置。默认：auto"),
    ZH_HANT("共視關係的來源。auto 優先用模型自帶的軌跡，否則把種子點雲投影到每一幀，再否則只用相機位置。預設：auto"),
    KO("- 공가시성의 출처.\n- auto는 모델의 트랙을 쓰고, 없으면 시드 점 구름을 모든 프레임에 투영하며, 그것도 없으면 카메라 위치만 씁니다.\n- 기본값: auto"),
    DE("Woher die Kovisibilität kommt. auto nimmt die Spuren des Modells, sonst projiziert es die Startpunktwolke in jedes Bild, sonst nur die Kamerapositionen. Standard: auto"),
    FR("D'où vient la covisibilité. auto prend les pistes du modèle, sinon projette le nuage d'amorce dans chaque image, sinon n'utilise que les positions des caméras. Défaut : auto"),
    ES("- De dónde sale la covisibilidad. auto toma las pistas del modelo; si no, proyecta la nube semilla en cada imagen;\n- si no, solo las posiciones de las cámaras. Predeterminado: auto"),
    PT("- De onde vem a covisibilidade. auto usa as pistas do modelo; senão, projeta a nuvem semente em cada imagem;\n- senão, só as posições das câmaras. Predefinição: auto"),
    IT("auto (default): usa tracce del modello, altrimenti proietta i punti iniziali nei fotogrammi; senza punti usa le posizioni delle camere."),
    NL("- Waar covisibiliteit vandaan komt.\n- auto neemt de sporen van het model, anders projecteert het de zaadwolk in elk beeld, anders alleen de cameraposities.\n- Standaard: auto"),
    RU("- Откуда берётся совидимость.\n- auto берёт треки модели, иначе проецирует начальное облако в каждый кадр, иначе использует только положения камер.\n- По умолчанию: auto"),
    TR("Ortak görünürlüğün kaynağı. auto modelin kendi izlerini alır, yoksa tohum bulutunu her kareye yansıtır, o da yoksa yalnız kamera konumlarını kullanır. Varsayılan: auto"));

SS_MSG(opt_output,
    EN("Where to write the partition. Default: <dataset>/partition.json"),
    JA("分割の書き出し先。既定: <dataset>/partition.json"),
    ZH_HANS("分区文件的写出位置。默认：<dataset>/partition.json"),
    ZH_HANT("分區檔案的寫出位置。預設：<dataset>/partition.json"),
    KO("분할을 기록할 위치. 기본값: <dataset>/partition.json"),
    DE("Wohin die Partition geschrieben wird. Standard: <dataset>/partition.json"),
    FR("Où écrire la partition. Défaut : <dataset>/partition.json"),
    ES("Dónde escribir la partición. Predeterminado: <dataset>/partition.json"),
    PT("Onde escrever a partição. Predefinição: <dataset>/partition.json"),
    IT("Dove scrivere la partizione. Predefinito: <dataset>/partition.json"),
    NL("Waar de partitie wordt geschreven. Standaard: <dataset>/partition.json"),
    RU("Куда записать разбиение. По умолчанию: <dataset>/partition.json"),
    TR("Bölümlemenin yazılacağı yer. Varsayılan: <dataset>/partition.json"));

SS_MSG(opt_max_projected,
    EN("Maximum points sampled for projection covisibility. Default: 200000."),
    JA("投影共視性の点サンプル上限。既定: 200000。"),
    ZH_HANS("投影共视关系的最大采样点数。默认：200000。"),
    ZH_HANT("投影共視關係的最大採樣點數。預設：200000。"),
    KO("투영 공가시성의 최대 샘플 점 수. 기본값: 200000."),
    DE("Maximale Punktzahl für projizierte Kovisibilität. Standard: 200000."),
    FR("Points échantillonnés pour la covisibilité projetée. Maximum : 200000."),
    ES("Máximo de puntos para covisibilidad proyectada. Predeterminado: 200000."),
    PT("Máximo de pontos para covisibilidade projetada. Padrão: 200000."),
    IT("Massimo di punti per covisibilità proiettata. Default: 200000."),
    NL("Maximaal punten voor geprojecteerde covisibiliteit. Standaard: 200000."),
    RU("Предел точек для проекции совидимости. По умолчанию: 200000."),
    TR("Yansıtılan ortak görünürlük için azami nokta. Varsayılan: 200000."));

SS_MSG(opt_projection_depth,
    EN("Projected occlusion depth slack; 0 off, 0.1 = 10% (experimental)."),
    JA("投影遮蔽の深度余裕。0 は無効、0.1 は 10%（実験的）。"),
    ZH_HANS("投影遮挡深度余量；0 关闭，0.1 = 10%（实验性）。"),
    ZH_HANT("投影遮擋深度餘量；0 關閉，0.1 = 10%（實驗性）。"),
    KO("투영 가림 깊이 여유. 0 끄기, 0.1 = 10% (실험적)."),
    DE("Tiefentoleranz für projizierte Verdeckung; 0 aus, 0.1 = 10% (experimentell)."),
    FR("Marge de profondeur pour l'occlusion projetée ; 0 désactive, 0.1 = 10% (expérimental)."),
    ES("Margen de profundidad de oclusión proyectada; 0 desactiva, 0.1 = 10% (experimental)."),
    PT("Margem de profundidade da oclusão projetada; 0 desliga, 0.1 = 10% (experimental)."),
    IT("Margine di profondità per l'occlusione proiettata; 0 disattiva, 0.1 = 10% (sperimentale)."),
    NL("Dieptemarge voor geprojecteerde occlusie; 0 uit, 0.1 = 10% (experimenteel)."),
    RU("Запас глубины проекционного перекрытия; 0 выкл., 0.1 = 10% (экспериментально)."),
    TR("Yansıtılan örtülme derinliği payı; 0 kapalı, 0.1 = %10 (deneysel)."));

SS_MSG(opt_runs,
    EN("Folder of training runs to find the parts' models in, by their "
       "config.json. Default: <dataset>/outputs"),
    JA("各パートのモデルを config.json から探す学習実行のフォルダ。既定: "
       "<dataset>/outputs"),
    ZH_HANS("根据 config.json 查找各分区模型的训练输出文件夹。默认：<dataset>/outputs"),
    ZH_HANT("根據 config.json 查找各分區模型的訓練輸出資料夾。預設：<dataset>/outputs"),
    KO("config.json으로 각 파트의 모델을 찾을 학습 실행 폴더. 기본값: "
       "<dataset>/outputs"),
    DE("Ordner mit Trainingsläufen, in dem die Modelle der Teile über ihre "
       "config.json gesucht werden. Standard: <dataset>/outputs"),
    FR("Dossier des entraînements où trouver les modèles des parties, via leur "
       "config.json. Défaut : <dataset>/outputs"),
    ES("Carpeta de ejecuciones donde buscar los modelos de las partes por su "
       "config.json. Predeterminado: <dataset>/outputs"),
    PT("Pasta de treinos onde procurar os modelos das partes pelo seu "
       "config.json. Predefinição: <dataset>/outputs"),
    IT("Cartella delle esecuzioni in cui cercare i modelli delle parti dal loro "
       "config.json. Predefinito: <dataset>/outputs"),
    NL("Map met trainingsruns waarin de modellen van de delen via hun config.json "
       "worden gezocht. Standaard: <dataset>/outputs"),
    RU("Папка с запусками обучения, где по config.json ищутся модели частей. "
       "По умолчанию: <dataset>/outputs"),
    TR("Parçaların modellerinin config.json'larından bulunacağı eğitim "
       "çalıştırmaları klasörü. Varsayılan: <dataset>/outputs"));

SS_MSG(opt_merge_output,
    EN("The merged model file. Default: merged.ply beside the partition"),
    JA("結合したモデルのファイル。既定: 分割ファイルの隣の merged.ply"),
    ZH_HANS("合并后的模型文件。默认：分区文件旁的 merged.ply"),
    ZH_HANT("合併後的模型檔案。預設：分區檔案旁的 merged.ply"),
    KO("병합된 모델 파일. 기본값: 분할 파일 옆의 merged.ply"),
    DE("Die zusammengeführte Modelldatei. Standard: merged.ply neben der Partition"),
    FR("Le fichier du modèle fusionné. Défaut : merged.ply à côté de la partition"),
    ES("El archivo del modelo fusionado. Predeterminado: merged.ply junto a la partición"),
    PT("O ficheiro do modelo fundido. Predefinição: merged.ply ao lado da partição"),
    IT("Il file del modello unito. Predefinito: merged.ply accanto alla partizione"),
    NL("Het samengevoegde modelbestand. Standaard: merged.ply naast de partitie"),
    RU("Файл объединённой модели. По умолчанию: merged.ply рядом с разбиением"),
    TR("Birleştirilmiş model dosyası. Varsayılan: bölümlemenin yanındaki merged.ply"));

SS_MSG(opt_merge_part,
    EN("Part k's run folder or splat.ply, instead of searching --runs; "
       "repeatable"),
    JA("--runs を探す代わりに指定するパート k の実行フォルダまたは splat.ply。複"
       "数指定可"),
    ZH_HANS("直接指定分区 k 的运行文件夹或 splat.ply，而不在 --runs 中搜索；可重复"),
    ZH_HANT("直接指定分區 k 的執行資料夾或 splat.ply，而不在 --runs 中搜尋；可重複"),
    KO("--runs를 찾는 대신 파트 k의 실행 폴더나 splat.ply를 직접 지정. 반복 가능"),
    DE("Laufordner oder splat.ply von Teil k statt der Suche in --runs; mehrfach "
       "möglich"),
    FR("Dossier d'entraînement ou splat.ply de la partie k, au lieu de chercher "
       "dans --runs ; répétable"),
    ES("Carpeta de ejecución o splat.ply de la parte k, en lugar de buscar en "
       "--runs; repetible"),
    PT("Pasta de treino ou splat.ply da parte k, em vez de procurar em --runs; "
       "repetível"),
    IT("Cartella di esecuzione o splat.ply della parte k, invece di cercare in "
       "--runs; ripetibile"),
    NL("Runmap of splat.ply van deel k, in plaats van zoeken in --runs; herhaalbaar"),
    RU("Папка запуска или splat.ply части k вместо поиска в --runs; можно "
       "повторять"),
    TR("--runs içinde aramak yerine k parçasının çalıştırma klasörü ya da "
       "splat.ply'si; yinelenebilir"));

SS_MSG(label_common,
    EN("Common:"), JA("共通:"), ZH_HANS("通用："), ZH_HANT("通用："), KO("공통:"),
    DE("Allgemein:"), FR("Commun :"), ES("Común:"), PT("Comum:"), IT("Comune:"),
    NL("Algemeen:"), RU("Общие:"), TR("Ortak:"));

// ===========================================================================
// Errors and log lines
// ===========================================================================

SS_MSG(err_usage,
    EN("Usage: {0} split <dataset> [options]   or   {0} merge <partition.json> [options]"),
    JA("使い方: {0} split <dataset> [options]   または   {0} merge <partition.json> [options]"),
    ZH_HANS("用法：{0} split <dataset> [options]   或   {0} merge <partition.json> [options]"),
    ZH_HANT("用法：{0} split <dataset> [options]   或   {0} merge <partition.json> [options]"),
    KO("사용법: {0} split <dataset> [options]   또는   {0} merge <partition.json> [options]"),
    DE("Aufruf: {0} split <dataset> [options]   oder   {0} merge <partition.json> [options]"),
    FR("Usage : {0} split <dataset> [options]   ou   {0} merge <partition.json> [options]"),
    ES("Uso: {0} split <dataset> [options]   o   {0} merge <partition.json> [options]"),
    PT("Uso: {0} split <dataset> [options]   ou   {0} merge <partition.json> [options]"),
    IT("Uso: {0} split <dataset> [options]   oppure   {0} merge <partition.json> [options]"),
    NL("Gebruik: {0} split <dataset> [options]   of   {0} merge <partition.json> [options]"),
    RU("Использование: {0} split <dataset> [options]   или   {0} merge <partition.json> [options]"),
    TR("Kullanım: {0} split <dataset> [options]   ya da   {0} merge <partition.json> [options]"));

SS_MSG(err_bad_value,
    EN("{0}: not a valid value for {1}"),
    JA("{0}: {1} の値として無効です"),
    ZH_HANS("{0}：不是 {1} 的有效值"),
    ZH_HANT("{0}：不是 {1} 的有效值"),
    KO("{0}: {1}에 쓸 수 없는 값입니다"),
    DE("{0}: kein gültiger Wert für {1}"),
    FR("{0} : valeur invalide pour {1}"),
    ES("{0}: no es un valor válido para {1}"),
    PT("{0}: não é um valor válido para {1}"),
    IT("{0}: non è un valore valido per {1}"),
    NL("{0}: geen geldige waarde voor {1}"),
    RU("{0}: недопустимое значение для {1}"),
    TR("{0}: {1} için geçerli bir değer değil"));

SS_MSG(log_reading,
    EN("Reading {0}"), JA("{0} を読み込み中"), ZH_HANS("正在读取 {0}"),
    ZH_HANT("正在讀取 {0}"), KO("{0} 읽는 중"), DE("Lese {0}"), FR("Lecture de {0}"),
    ES("Leyendo {0}"), PT("A ler {0}"), IT("Lettura di {0}"), NL("{0} wordt gelezen"),
    RU("Чтение {0}"), TR("{0} okunuyor"));

SS_MSG(log_dataset,
    EN("Cameras: {0}   points: {1}   tracks: {2}"),
    JA("カメラ: {0}   点: {1}   トラック: {2}"),
    ZH_HANS("相机：{0}   点：{1}   轨迹：{2}"),
    ZH_HANT("相機：{0}   點：{1}   軌跡：{2}"),
    KO("카메라: {0}   점: {1}   트랙: {2}"),
    DE("Kameras: {0}   Punkte: {1}   Spuren: {2}"),
    FR("Caméras : {0}   points : {1}   pistes : {2}"),
    ES("Cámaras: {0}   puntos: {1}   pistas: {2}"),
    PT("Câmaras: {0}   pontos: {1}   pistas: {2}"),
    IT("Fotocamere: {0}   punti: {1}   tracce: {2}"),
    NL("Camera's: {0}   punten: {1}   sporen: {2}"),
    RU("Камер: {0}   точек: {1}   треков: {2}"),
    TR("Kamera: {0}   nokta: {1}   iz: {2}"));

SS_MSG(word_yes, EN("yes"), JA("あり"), ZH_HANS("有"), ZH_HANT("有"), KO("있음"),
    DE("ja"), FR("oui"), ES("sí"), PT("sim"), IT("sì"), NL("ja"), RU("есть"), TR("var"));
SS_MSG(word_no, EN("no"), JA("なし"), ZH_HANS("无"), ZH_HANT("無"), KO("없음"),
    DE("nein"), FR("non"), ES("no"), PT("não"), IT("no"), NL("nee"), RU("нет"), TR("yok"));

SS_MSG(log_covisibility,
    EN("Covisibility from {0}: camera pairs {1}"),
    JA("{0} による共視性: カメラ対 {1}"),
    ZH_HANS("来自 {0} 的共视关系：相机对 {1}"),
    ZH_HANT("來自 {0} 的共視關係：相機對 {1}"),
    KO("{0} 기반 공가시성: 카메라 쌍 {1}"),
    DE("Kovisibilität aus {0}: Kamerapaare {1}"),
    FR("Covisibilité issue de {0} : paires de caméras {1}"),
    ES("Covisibilidad a partir de {0}: pares de cámaras {1}"),
    PT("Covisibilidade a partir de {0}: pares de câmaras {1}"),
    IT("Covisibilità da {0}: coppie di fotocamere {1}"),
    NL("Covisibiliteit uit {0}: cameraparen {1}"),
    RU("Совидимость из {0}: пар камер {1}"),
    TR("{0} kaynaklı ortak görünürlük: kamera çifti {1}"));

SS_MSG(log_covisibility_without_graph,
    EN("Covisibility from {0}: camera pairs not stored"),
    JA("{0} による共視性: カメラ対は保存しません"),
    ZH_HANS("来自 {0} 的共视关系：不保存相机对"),
    ZH_HANT("來自 {0} 的共視關係：不保存相機對"),
    KO("{0} 기반 공가시성: 카메라 쌍 저장 안 함"),
    DE("Kovisibilität aus {0}: Kamerapaare nicht gespeichert"),
    FR("Covisibilité issue de {0} : paires de caméras non stockées"),
    ES("Covisibilidad a partir de {0}: pares de cámaras sin almacenar"),
    PT("Covisibilidade a partir de {0}: pares de câmaras não guardados"),
    IT("Covisibilità da {0}: coppie di fotocamere non memorizzate"),
    NL("Covisibiliteit uit {0}: cameraparen niet opgeslagen"),
    RU("Совидимость из {0}: пары камер не сохранены"),
    TR("{0} kaynaklı ortak görünürlük: kamera çiftleri saklanmadı"));

SS_MSG(log_summary,
    EN("Parts: {0}   covisibility cut across parts: {1}%   ownership field seeds: {2}"),
    JA("パート: {0}   パート間で切れた共視性: {1}%   所有領域フィールドの点: {2}"),
    ZH_HANS("分区：{0}   被切断的共视关系：{1}%   归属场种子点：{2}"),
    ZH_HANT("分區：{0}   被切斷的共視關係：{1}%   歸屬場種子點：{2}"),
    KO("파트: {0}   파트 사이에서 잘린 공가시성: {1}%   소유 필드 시드: {2}"),
    DE("Teile: {0}   zwischen Teilen durchtrennte Kovisibilität: {1}%   Startpunkte des Zugehörigkeitsfelds: {2}"),
    FR("Parties : {0}   covisibilité coupée entre parties : {1}%   points du champ d'appartenance : {2}"),
    ES("Partes: {0}   covisibilidad cortada entre partes: {1}%   semillas del campo de pertenencia: {2}"),
    PT("Partes: {0}   covisibilidade cortada entre partes: {1}%   sementes do campo de pertença: {2}"),
    IT("Parti: {0}   covisibilità tagliata tra le parti: {1}%   semi del campo di appartenenza: {2}"),
    NL("Delen: {0}   covisibiliteit doorgesneden tussen delen: {1}%   zaden van het eigendomsveld: {2}"),
    RU("Частей: {0}   совидимость, разрезанная между частями: {1}%   точек поля принадлежности: {2}"),
    TR("Parça: {0}   parçalar arasında kesilen ortak görünürlük: %{1}   aidiyet alanı tohumu: {2}"));

SS_MSG(log_part,
    EN("  part {0}: core {1}   ring {2}   seed points {3}   its cameras see {4}% of it"),
    JA("  パート {0}: コア {1}   リング {2}   初期点 {3}   カメラの視野のうち自パート {4}%"),
    ZH_HANS("  分区 {0}：核心 {1}   外环 {2}   种子点 {3}   相机视野中本分区占 {4}%"),
    ZH_HANT("  分區 {0}：核心 {1}   外環 {2}   種子點 {3}   相機視野中本分區佔 {4}%"),
    KO("  파트 {0}: 핵심 {1}   고리 {2}   시드 점 {3}   카메라 시야 중 자기 파트 {4}%"),
    DE("  Teil {0}: Kern {1}   Ring {2}   Startpunkte {3}   eigener Anteil der Sicht {4}%"),
    FR("  partie {0} : cœur {1}   anneau {2}   points d'amorce {3}   part propre de la vue {4} %"),
    ES("  parte {0}: núcleo {1}   anillo {2}   puntos semilla {3}   parte propia de la vista {4}%"),
    PT("  parte {0}: núcleo {1}   anel {2}   pontos semente {3}   parte própria da vista {4}%"),
    IT("  parte {0}: nucleo {1}   anello {2}   punti seme {3}   quota propria della vista {4}%"),
    NL("  deel {0}: kern {1}   ring {2}   zaadpunten {3}   eigen deel van het zicht {4}%"),
    RU("  часть {0}: ядро {1}   кольцо {2}   начальных точек {3}   своя доля обзора {4}%"),
    TR("  parça {0}: çekirdek {1}   halka {2}   tohum noktası {3}   görüşün kendi payı %{4}"));

SS_MSG(log_part_pieces,
    EN("  part {0} is not one piece of the view graph but {1}"),
    JA("  パート {0} はビューグラフ上でひとつながりではなく {1} 個に分かれています"),
    ZH_HANS("  分区 {0} 在视图图中不是一块，而是 {1} 块"),
    ZH_HANT("  分區 {0} 在視圖圖中不是一塊，而是 {1} 塊"),
    KO("  파트 {0}은(는) 뷰 그래프에서 한 덩어리가 아니라 {1} 덩어리입니다"),
    DE("  Teil {0} ist im Sichtgraphen kein zusammenhängendes Stück, sondern {1}"),
    FR("  la partie {0} n'est pas d'un seul tenant dans le graphe de vues mais {1}"),
    ES("  la parte {0} no es una sola pieza del grafo de vistas sino {1}"),
    PT("  a parte {0} não é uma só peça do grafo de vistas mas {1}"),
    IT("  la parte {0} non è un solo pezzo del grafo delle viste ma {1}"),
    NL("  deel {0} is geen enkel stuk van de zichtgraaf maar {1}"),
    RU("  часть {0} в графе видов состоит не из одного куска, а из {1}"),
    TR("  parça {0} görüş grafında tek parça değil, {1} parça"));

SS_MSG(log_written,
    EN("Partition written: {0}"), JA("分割を書き出しました: {0}"),
    ZH_HANS("已写出分区：{0}"), ZH_HANT("已寫出分區：{0}"), KO("분할을 기록했습니다: {0}"),
    DE("Partition geschrieben: {0}"), FR("Partition écrite : {0}"),
    ES("Partición escrita: {0}"), PT("Partição escrita: {0}"),
    IT("Partizione scritta: {0}"), NL("Partitie geschreven: {0}"),
    RU("Разбиение записано: {0}"), TR("Bölümleme yazıldı: {0}"));

SS_MSG(log_next,
    EN("Train part k with: {0} train --data {1} --partition {2} --partition-part k"),
    JA("パート k の学習: {0} train --data {1} --partition {2} --partition-part k"),
    ZH_HANS("训练分区 k：{0} train --data {1} --partition {2} --partition-part k"),
    ZH_HANT("訓練分區 k：{0} train --data {1} --partition {2} --partition-part k"),
    KO("파트 k 학습: {0} train --data {1} --partition {2} --partition-part k"),
    DE("Teil k trainieren mit: {0} train --data {1} --partition {2} --partition-part k"),
    FR("Entraîner la partie k : {0} train --data {1} --partition {2} --partition-part k"),
    ES("Entrenar la parte k: {0} train --data {1} --partition {2} --partition-part k"),
    PT("Treinar a parte k: {0} train --data {1} --partition {2} --partition-part k"),
    IT("Addestrare la parte k: {0} train --data {1} --partition {2} --partition-part k"),
    NL("Deel k trainen met: {0} train --data {1} --partition {2} --partition-part k"),
    RU("Обучить часть k: {0} train --data {1} --partition {2} --partition-part k"),
    TR("k parçasını eğitmek için: {0} train --data {1} --partition {2} --partition-part k"));

SS_MSG(merge_part_missing,
    EN("  part {0}: no trained model found"),
    JA("  パート {0}: 学習済みモデルが見つかりません"),
    ZH_HANS("  分区 {0}：没有找到训练好的模型"),
    ZH_HANT("  分區 {0}：沒有找到訓練好的模型"),
    KO("  파트 {0}: 학습된 모델이 없습니다"),
    DE("  Teil {0}: kein trainiertes Modell gefunden"),
    FR("  partie {0} : aucun modèle entraîné trouvé"),
    ES("  parte {0}: no se encontró ningún modelo entrenado"),
    PT("  parte {0}: nenhum modelo treinado encontrado"),
    IT("  parte {0}: nessun modello addestrato trovato"),
    NL("  deel {0}: geen getraind model gevonden"),
    RU("  часть {0}: обученная модель не найдена"),
    TR("  parça {0}: eğitilmiş model bulunamadı"));

SS_MSG(merge_part,
    EN("  part {0}: splats {1}   inside its region {2}   from {3}"),
    JA("  パート {0}: スプラット {1}   自領域内 {2}   元 {3}"),
    ZH_HANS("  分区 {0}：泼溅 {1}   位于自身区域内 {2}   来自 {3}"),
    ZH_HANT("  分區 {0}：潑濺 {1}   位於自身區域內 {2}   來自 {3}"),
    KO("  파트 {0}: 스플랫 {1}   자기 영역 안 {2}   출처 {3}"),
    DE("  Teil {0}: Splats {1}   im eigenen Bereich {2}   aus {3}"),
    FR("  partie {0} : splats {1}   dans sa région {2}   depuis {3}"),
    ES("  parte {0}: splats {1}   dentro de su región {2}   de {3}"),
    PT("  parte {0}: splats {1}   dentro da sua região {2}   de {3}"),
    IT("  parte {0}: splat {1}   dentro la sua regione {2}   da {3}"),
    NL("  deel {0}: splats {1}   binnen zijn gebied {2}   uit {3}"),
    RU("  часть {0}: сплатов {1}   внутри своей области {2}   из {3}"),
    TR("  parça {0}: splat {1}   kendi bölgesinde {2}   kaynak {3}"));

SS_MSG(merge_written,
    EN("Merged model written: {0}   splats {1}"),
    JA("結合したモデルを書き出しました: {0}   スプラット {1}"),
    ZH_HANS("已写出合并模型：{0}   泼溅 {1}"),
    ZH_HANT("已寫出合併模型：{0}   潑濺 {1}"),
    KO("병합 모델을 기록했습니다: {0}   스플랫 {1}"),
    DE("Zusammengeführtes Modell geschrieben: {0}   Splats {1}"),
    FR("Modèle fusionné écrit : {0}   splats {1}"),
    ES("Modelo fusionado escrito: {0}   splats {1}"),
    PT("Modelo fundido escrito: {0}   splats {1}"),
    IT("Modello unito scritto: {0}   splat {1}"),
    NL("Samengevoegd model geschreven: {0}   splats {1}"),
    RU("Объединённая модель записана: {0}   сплатов {1}"),
    TR("Birleştirilmiş model yazıldı: {0}   splat {1}"));

SS_MSG(err_nothing_to_merge,
    EN("No part has a trained model; nothing to merge."),
    JA("学習済みモデルのあるパートがなく、結合するものがありません。"),
    ZH_HANS("没有任何分区有训练好的模型，无可合并。"),
    ZH_HANT("沒有任何分區有訓練好的模型，無可合併。"),
    KO("학습된 모델이 있는 파트가 없어 병합할 것이 없습니다."),
    DE("Kein Teil hat ein trainiertes Modell; nichts zusammenzuführen."),
    FR("Aucune partie n'a de modèle entraîné ; rien à fusionner."),
    ES("Ninguna parte tiene un modelo entrenado; nada que fusionar."),
    PT("Nenhuma parte tem um modelo treinado; nada a fundir."),
    IT("Nessuna parte ha un modello addestrato; niente da unire."),
    NL("Geen deel heeft een getraind model; niets samen te voegen."),
    RU("Ни у одной части нет обученной модели; объединять нечего."),
    TR("Hiçbir parçanın eğitilmiş modeli yok; birleştirilecek bir şey yok."));

// ===========================================================================
// The GUI panel
// ===========================================================================

SS_MSG(panel_title,
    EN("Partition"), JA("分割"), ZH_HANS("分区"), ZH_HANT("分區"), KO("분할"),
    DE("Partition"), FR("Partition"), ES("Partición"), PT("Partição"),
    IT("Partizione"), NL("Partitie"), RU("Разбиение"), TR("Bölümleme"));

SS_MSG(chip_part,
    EN("part {0}"), JA("パート {0}"), ZH_HANS("分区 {0}"), ZH_HANT("分區 {0}"),
    KO("파트 {0}"), DE("Teil {0}"), FR("partie {0}"), ES("parte {0}"), PT("parte {0}"),
    IT("parte {0}"), NL("deel {0}"), RU("часть {0}"), TR("parça {0}"));

SS_MSG(open_button,
    EN("Partition"), JA("分割"), ZH_HANS("分区"), ZH_HANT("分區"), KO("분할"),
    DE("Partitionieren"), FR("Partitionner"), ES("Particionar"), PT("Particionar"),
    IT("Partiziona"), NL("Partitioneren"), RU("Разбить"), TR("Bölümle"));

SS_MSG(open_button_help,
    EN("Split this reconstruction into parts that train one at a time on this "
       "GPU, queue them in Batch, and merge the models back into one when they "
       "are done."),
    JA("この再構成を、この GPU で 1 つずつ学習するパートに分割し、バッチに並べ、"
       "終わったらモデルをひとつに結合します。"),
    ZH_HANS("把这次重建拆成可在本 GPU 上逐个训练的分区，排进批处理，训练完后再把模型"
            "合并成一个。"),
    ZH_HANT("把這次重建拆成可在本 GPU 上逐個訓練的分區，排進批次處理，訓練完後再把模型"
            "合併成一個。"),
    KO("이 재구성을 이 GPU에서 하나씩 학습할 파트로 나누고, 배치에 넣고, 끝나면 "
       "모델을 다시 하나로 병합합니다."),
    DE("Diese Rekonstruktion in Teile zerlegen, die auf dieser GPU nacheinander "
       "trainieren, sie in den Stapel stellen und die Modelle danach wieder zu "
       "einem zusammenführen."),
    FR("Découper cette reconstruction en parties entraînées une à une sur ce GPU, "
       "les mettre en file dans Lot, puis refusionner les modèles en un seul."),
    ES("Dividir esta reconstrucción en partes que se entrenan una a una en esta "
       "GPU, ponerlas en cola en Lote y fusionar los modelos en uno al terminar."),
    PT("Dividir esta reconstrução em partes treinadas uma a uma nesta GPU, "
       "pô-las em fila no Lote e voltar a fundir os modelos num só no fim."),
    IT("Dividere questa ricostruzione in parti addestrate una alla volta su questa "
       "GPU, metterle in coda nel Lotto e riunire i modelli in uno alla fine."),
    NL("Deze reconstructie splitsen in delen die één voor één op deze GPU trainen, "
       "ze in Batch zetten en de modellen daarna weer tot één samenvoegen."),
    RU("Разбить эту реконструкцию на части, обучаемые по одной на этой GPU, "
       "поставить их в пакет и потом объединить модели в одну."),
    TR("Bu yeniden oluşturmayı bu GPU'da tek tek eğitilecek parçalara bölmek, "
       "Toplu İşe sıralamak ve bitince modelleri yeniden tek modelde birleştirmek."));

SS_MSG(status_reading,
    EN("Reading the reconstruction..."), JA("再構成を読み込んでいます…"),
    ZH_HANS("正在读取重建…"), ZH_HANT("正在讀取重建…"), KO("재구성을 읽는 중..."),
    DE("Rekonstruktion wird gelesen ..."), FR("Lecture de la reconstruction..."),
    ES("Leyendo la reconstrucción..."), PT("A ler a reconstrução..."),
    IT("Lettura della ricostruzione..."), NL("Reconstructie wordt gelezen..."),
    RU("Чтение реконструкции..."), TR("Yeniden oluşturma okunuyor..."));

SS_MSG(status_computing,
    EN("Computing the partition..."), JA("分割を計算しています…"),
    ZH_HANS("正在计算分区…"), ZH_HANT("正在計算分區…"), KO("분할을 계산하는 중..."),
    DE("Partition wird berechnet ..."), FR("Calcul de la partition..."),
    ES("Calculando la partición..."), PT("A calcular a partição..."),
    IT("Calcolo della partizione..."), NL("Partitie wordt berekend..."),
    RU("Вычисление разбиения..."), TR("Bölümleme hesaplanıyor..."));

SS_MSG(status_cancelled,
    EN("Cancelled; the partition shown is the previous one, if any."),
    JA("中止しました。表示中の分割は前回のもの（あれば）です。"),
    ZH_HANS("已取消；显示的是之前的分区（如果有）。"),
    ZH_HANT("已取消；顯示的是先前的分區（如果有）。"),
    KO("취소했습니다. 보이는 분할은 이전 것입니다(있다면)."),
    DE("Abgebrochen; angezeigt wird die vorige Partition, falls es eine gibt."),
    FR("Annulé ; la partition affichée est la précédente, s'il y en a une."),
    ES("Cancelado; la partición mostrada es la anterior, si la hay."),
    PT("Cancelado; a partição mostrada é a anterior, se houver."),
    IT("Annullato; la partizione mostrata è quella precedente, se c'è."),
    NL("Geannuleerd; de getoonde partitie is de vorige, als die er is."),
    RU("Отменено; показано предыдущее разбиение, если оно было."),
    TR("İptal edildi; gösterilen bölümleme, varsa, öncekidir."));

SS_MSG(warn_no_tracks,
    EN("No feature tracks: visibility is estimated by projecting points into images.\nFor better partitions, use a COLMAP model with sparse/0/images.bin and points3D.bin."),
    JA("特徴トラックがないため、点を画像へ投影して可視性を推定します。\nより良い分割には sparse/0/images.bin と points3D.bin を持つCOLMAPモデルを使ってください。"),
    ZH_HANS("无特征轨迹，使用点云投影估算可见性。\n使用含 sparse/0/images.bin 和 points3D.bin 的 COLMAP 模型可获得更好的分区。"),
    ZH_HANT("無特徵軌跡，使用點雲投影估算可見性。\n使用含 sparse/0/images.bin 和 points3D.bin 的 COLMAP 模型可獲得更好的分區。"),
    KO("특징 트랙이 없어 점을 이미지에 투영해 가시성을 추정합니다.\n더 나은 분할을 위해 sparse/0/images.bin과 points3D.bin이 있는 COLMAP 모델을 쓰세요."),
    DE("Keine Feature-Tracks: Sichtbarkeit wird durch Projektion der Punkte geschätzt.\nBessere Teile mit COLMAP-Modell und sparse/0/images.bin sowie points3D.bin."),
    FR("Pas de pistes : visibilité estimée par projection des points.\nPour mieux découper, utilisez un modèle COLMAP avec sparse/0/images.bin et points3D.bin."),
    ES("Sin tracks: visibilidad estimada proyectando puntos.\nPara mejores partes, usa un modelo COLMAP con sparse/0/images.bin y points3D.bin."),
    PT("Sem tracks: visibilidade estimada projetando pontos.\nPara partes melhores, use um modelo COLMAP com sparse/0/images.bin e points3D.bin."),
    IT("Senza tracce: visibilità stimata proiettando i punti.\nPer parti migliori usa un modello COLMAP con sparse/0/images.bin e points3D.bin."),
    NL("Geen featuretracks: zichtbaarheid geschat door punten te projecteren.\nGebruik voor betere delen een COLMAP-model met sparse/0/images.bin en points3D.bin."),
    RU("Нет треков: видимость оценивается проекцией точек.\nДля лучшего деления используйте COLMAP-модель с sparse/0/images.bin и points3D.bin."),
    TR("Özellik izleri yok: görünürlük noktalar izdüşürülerek tahmin edilir.\nDaha iyi parçalar için sparse/0/images.bin ve points3D.bin içeren COLMAP modeli kullanın."));

SS_MSG(status_summary,
    EN("Parts: {0}   cameras: {1}   points: {2}   covisibility cut: {3}%   from {4}"),
    JA("パート: {0}   カメラ: {1}   点: {2}   切れた共視性: {3}%   出どころ {4}"),
    ZH_HANS("分区：{0}   相机：{1}   点：{2}   被切断的共视关系：{3}%   来源 {4}"),
    ZH_HANT("分區：{0}   相機：{1}   點：{2}   被切斷的共視關係：{3}%   來源 {4}"),
    KO("파트: {0}   카메라: {1}   점: {2}   잘린 공가시성: {3}%   출처 {4}"),
    DE("Teile: {0}   Kameras: {1}   Punkte: {2}   durchtrennte Kovisibilität: {3}%   aus {4}"),
    FR("Parties : {0}   caméras : {1}   points : {2}   covisibilité coupée : {3}%   source {4}"),
    ES("Partes: {0}   cámaras: {1}   puntos: {2}   covisibilidad cortada: {3}%   de {4}"),
    PT("Partes: {0}   câmaras: {1}   pontos: {2}   covisibilidade cortada: {3}%   de {4}"),
    IT("Parti: {0}   fotocamere: {1}   punti: {2}   covisibilità tagliata: {3}%   da {4}"),
    NL("Delen: {0}   camera's: {1}   punten: {2}   doorgesneden covisibiliteit: {3}%   uit {4}"),
    RU("Частей: {0}   камер: {1}   точек: {2}   разрезанная совидимость: {3}%   из {4}"),
    TR("Parça: {0}   kamera: {1}   nokta: {2}   kesilen ortak görünürlük: %{3}   kaynak {4}"));

SS_MSG(status_lcc,
    EN("{0} blocks, {1} cameras, {2} points"),
    JA("{0} ブロック、{1} カメラ、{2} 点"),
    ZH_HANS("{0} 个分块，{1} 个相机，{2} 个点"),
    ZH_HANT("{0} 個分塊，{1} 個相機，{2} 個點"),
    KO("블록 {0}개, 카메라 {1}개, 점 {2}개"),
    DE("{0} Blöcke, {1} Kameras, {2} Punkte"),
    FR("{0} blocs, {1} caméras, {2} points"),
    ES("{0} bloques, {1} cámaras, {2} puntos"),
    PT("{0} blocos, {1} câmaras, {2} pontos"),
    IT("{0} blocchi, {1} fotocamere, {2} punti"),
    NL("{0} blokken, {1} camera's, {2} punten"),
    RU("{0} блоков, {1} камер, {2} точек"),
    TR("{0} blok, {1} kamera, {2} nokta"));

SS_MSG(col_gpixels,
    EN("GPixels"), JA("GPixels"), ZH_HANS("GPixels"), ZH_HANT("GPixels"),
    KO("GPixels"), DE("GPixels"), FR("GPixels"), ES("GPixels"), PT("GPixels"),
    IT("GPixels"), NL("GPixels"), RU("GPixels"), TR("GPixels"));

SS_MSG(lbl_method,
    EN("Split"), JA("分割方法"), ZH_HANS("切分方式"), ZH_HANT("切分方式"), KO("분할 방식"),
    DE("Aufteilung"), FR("Découpe"), ES("División"), PT("Divisão"), IT("Suddivisione"),
    NL("Opsplitsing"), RU("Разбиение"), TR("Bölme"));
SS_MSG(meth_graph,
    EN("Visibility cut"), JA("可視性カット"), ZH_HANS("可见性切分"), ZH_HANT("可見性切分"),
    KO("가시성 컷"), DE("Sichtbarkeitsschnitt"), FR("Coupe de visibilité"),
    ES("Corte de visibilidad"), PT("Corte de visibilidade"), IT("Taglio di visibilità"),
    NL("Zichtbaarheidssnede"), RU("Разрез по видимости"), TR("Görünürlük kesimi"));
SS_MSG(meth_viewgraph,
    EN("Cameras first (view graph)"), JA("カメラを先に（ビューグラフ）"),
    ZH_HANS("先分相机（视图图）"), ZH_HANT("先分相機（視圖圖）"), KO("카메라 먼저(뷰 그래프)"),
    DE("Kameras zuerst (Sichtgraph)"), FR("Caméras d'abord (graphe de vues)"),
    ES("Cámaras primero (grafo de vistas)"), PT("Câmaras primeiro (grafo de vistas)"),
    IT("Prima le fotocamere (grafo delle viste)"), NL("Eerst camera's (zichtgraaf)"),
    RU("Сначала камеры (граф видов)"), TR("Önce kameralar (görüş grafı)"));

SS_MSG(meth_voronoi,
    EN("Compact cells (Voronoi)"), JA("凸領域（ボロノイ）"),
    ZH_HANS("紧凑区域（Voronoi）"), ZH_HANT("緊湊區域（Voronoi）"),
    KO("조밀한 영역(Voronoi)"), DE("Kompakte Zellen (Voronoi)"),
    FR("Cellules compactes (Voronoi)"), ES("Celdas compactas (Voronoi)"),
    PT("Células compactas (Voronoi)"), IT("Celle compatte (Voronoi)"),
    NL("Compacte cellen (Voronoi)"), RU("Компактные ячейки (Вороного)"),
    TR("Kompakt hücreler (Voronoi)"));

SS_MSG(lbl_source,
    EN("Covisibility"), JA("共視性"), ZH_HANS("共视关系"), ZH_HANT("共視關係"),
    KO("공가시성"), DE("Kovisibilität"), FR("Covisibilité"), ES("Covisibilidad"),
    PT("Covisibilidade"), IT("Covisibilità"), NL("Covisibiliteit"), RU("Совидимость"),
    TR("Ortak görünürlük"));

SS_MSG(src_auto,
    EN("Automatic"), JA("自動"), ZH_HANS("自动"), ZH_HANT("自動"), KO("자동"),
    DE("Automatisch"), FR("Automatique"), ES("Automática"), PT("Automática"),
    IT("Automatica"), NL("Automatisch"), RU("Автоматически"), TR("Otomatik"));
SS_MSG(src_tracks,
    EN("Tracks of the model"), JA("モデルのトラック"), ZH_HANS("模型的轨迹"),
    ZH_HANT("模型的軌跡"), KO("모델의 트랙"), DE("Spuren des Modells"),
    FR("Pistes du modèle"), ES("Pistas del modelo"), PT("Pistas do modelo"),
    IT("Tracce del modello"), NL("Sporen van het model"), RU("Треки модели"),
    TR("Modelin izleri"));
SS_MSG(src_projection,
    EN("Seed cloud projected"), JA("初期点群の投影"), ZH_HANS("投影种子点云"),
    ZH_HANT("投影種子點雲"), KO("시드 점 구름 투영"), DE("Projizierte Startpunktwolke"),
    FR("Nuage d'amorce projeté"), ES("Nube semilla proyectada"),
    PT("Nuvem semente projetada"), IT("Nuvola seme proiettata"),
    NL("Geprojecteerde zaadwolk"), RU("Проекция начального облака"),
    TR("Yansıtılan tohum bulutu"));
SS_MSG(src_proximity,
    EN("Camera positions"), JA("カメラ位置"), ZH_HANS("相机位置"), ZH_HANT("相機位置"),
    KO("카메라 위치"), DE("Kamerapositionen"), FR("Positions des caméras"),
    ES("Posiciones de las cámaras"), PT("Posições das câmaras"),
    IT("Posizioni delle fotocamere"), NL("Cameraposities"), RU("Положения камер"),
    TR("Kamera konumları"));

SS_MSG(lbl_source_help,
    EN("What decides which cameras belong together. The model's own tracks are exact and free;\n- a dataset without them gets the seed cloud projected into every frame;\n- a dataset with no cloud is cut by where the cameras stand."),
    JA("どのカメラが同じパートに入るかを決めるもの。 モデル自身のトラックは正確で追加コストなし。\nトラックがないデータセットは初期点群を全フレームに投影し、点群もなければカメラの位置で切ります。"),
    ZH_HANS("决定哪些相机归为一组的依据。 模型自带的轨迹最准且无额外开销； 没有轨迹的数据集把种子点云投影到每一帧；\n连点云都没有的按相机位置切分。"),
    ZH_HANT("決定哪些相機歸為一組的依據。 模型自帶的軌跡最準且無額外開銷； 沒有軌跡的資料集把種子點雲投影到每一幀；\n連點雲都沒有的按相機位置切分。"),
    KO("모델 트랙이 있으면 그대로 씁니다. 없으면 초기 점군을 각 프레임에 투영하고, 점군도 없으면 카메라 위치로 나눕니다."),
    DE("Was entscheidet, welche Kameras zusammengehören. Die Spuren des Modells sind exakt und kostenlos;\n- ein Datensatz ohne sie bekommt die Startpunktwolke in jedes Bild projiziert;\n- einer ohne Wolke wird nach den Kamerastandorten geschnitten."),
    FR("Ce qui décide quelles caméras vont ensemble. Les pistes du modèle sont exactes et gratuites ;\nsans elles, le nuage d'amorce est projeté dans chaque image ; sans nuage, la coupe suit la position des caméras."),
    ES("Lo que decide qué cámaras van juntas. Las pistas del modelo son exactas y gratis;\nsin ellas se proyecta la nube semilla en cada imagen; sin nube, se corta por la posición de las cámaras."),
    PT("O que decide quais câmaras ficam juntas. As pistas do modelo são exatas e sem custo;\nsem elas a nuvem semente é projetada em cada imagem; sem nuvem, corta-se pela posição das câmaras."),
    IT("Ciò che decide quali fotocamere stanno insieme. Le tracce del modello sono esatte e gratuite;\n- senza di esse la nuvola seme viene proiettata in ogni immagine;\n- senza nuvola si taglia per la posizione delle fotocamere."),
    NL("Wat bepaalt welke camera's bij elkaar horen. De sporen van het model zijn exact en gratis;\nzonder die wordt de zaadwolk in elk beeld geprojecteerd; zonder wolk wordt gesneden op waar de camera's staan."),
    RU("Что решает, какие камеры относятся к одной части. Треки модели точны и бесплатны;\nбез них начальное облако проецируется в каждый кадр; без облака разрез идёт по положениям камер."),
    TR("Hangi kameraların birlikte olacağını ne belirler. Modelin izleri kesin ve bedavadır;\n- izi olmayan veri kümesinde tohum bulutu her kareye yansıtılır;\n- bulutu da olmayan kameraların durduğu yere göre kesilir."));

SS_MSG(lbl_split_by,
    EN("Split by"), JA("分割の基準"), ZH_HANS("拆分依据"), ZH_HANT("拆分依據"),
    KO("나누는 기준"), DE("Teilen nach"), FR("Découper par"), ES("Dividir por"),
    PT("Dividir por"), IT("Dividi per"), NL("Splitsen op"), RU("Делить по"),
    TR("Bölme ölçütü"));
SS_MSG(mode_parts,
    EN("Number of parts"), JA("パート数"), ZH_HANS("分区数量"), ZH_HANT("分區數量"),
    KO("파트 수"), DE("Anzahl Teile"), FR("Nombre de parties"), ES("Número de partes"),
    PT("Número de partes"), IT("Numero di parti"), NL("Aantal delen"),
    RU("Число частей"), TR("Parça sayısı"));
SS_MSG(mode_max,
    EN("Cameras per part"), JA("パートあたりのカメラ数"), ZH_HANS("每个分区的相机数"),
    ZH_HANT("每個分區的相機數"), KO("파트당 카메라 수"), DE("Kameras je Teil"),
    FR("Caméras par partie"), ES("Cámaras por parte"), PT("Câmaras por parte"),
    IT("Fotocamere per parte"), NL("Camera's per deel"), RU("Камер на часть"),
    TR("Parça başına kamera"));
SS_MSG(mode_help,
    EN("Choose a part count or a camera target per part. Overlapping cameras can exceed the target."),
    JA("パート数か各パートのカメラ数の目安を指定。重複カメラで目安を超える場合があります。"),
    ZH_HANS("选择分区数或每区相机数量目标。重叠相机可使数量超出目标。"),
    ZH_HANT("選擇分區數或每區相機數量目標。重疊相機可使數量超出目標。"),
    KO("파트 수 또는 파트별 카메라 수 목표를 선택. 중첩 카메라로 목표를 초과할 수 있습니다."),
    DE("Anzahl der Teile oder Kameraziel je Teil wählen. Überlappende Kameras können das Ziel überschreiten."),
    FR("Choisissez un nombre de parties ou une cible de caméras par partie. Le recouvrement peut dépasser la cible."),
    ES("Elija un número de partes o un objetivo de cámaras por parte. El solapamiento puede superar el objetivo."),
    PT("Escolha o número de partes ou uma meta de câmaras por parte. A sobreposição pode exceder a meta."),
    IT("Scegli il numero di parti o un obiettivo di fotocamere per parte. La sovrapposizione può superarlo."),
    NL("Kies een aantal delen of een cameradoel per deel. Overlappende camera's kunnen het doel overschrijden."),
    RU("Выберите число частей или цель по камерам на часть. Перекрывающиеся камеры могут превысить цель."),
    TR("Parça sayısı veya parça başına kamera hedefi seçin. Örtüşen kameralar hedefi aşabilir."));

SS_MSG(lbl_ring,
    EN("Ring share"), JA("リングの割合"), ZH_HANS("外环比例"), ZH_HANT("外環比例"),
    KO("고리 비율"), DE("Ringanteil"), FR("Part de l'anneau"), ES("Umbral del anillo"),
    PT("Limiar do anel"), IT("Soglia dell'anello"), NL("Ringaandeel"), RU("Порог кольца"),
    TR("Halka payı"));
SS_MSG(lbl_ring_help,
    EN("An outside camera trains with a part when at least this share of the points it sees belongs to the part.\nHigher means fewer borrowed cameras and faster parts; lower means seams seen from both sides by more views."),
    JA("外側のカメラは、見ている点のうちこの割合以上がパートに属するときそのパートと一緒に学習します。\n高くすると借りるカメラが減って速く、低くすると継ぎ目を両側から見る視点が増えます。"),
    ZH_HANS("外围相机所见点中属于该分区的比例达到此值时，与该分区一起训练。 值越高借用的相机越少、训练越快；\n值越低则有更多视角从两侧覆盖接缝。"),
    ZH_HANT("外圍相機所見點中屬於該分區的比例達到此值時，與該分區一起訓練。 值越高借用的相機越少、訓練越快；\n值越低則有更多視角從兩側覆蓋接縫。"),
    KO("바깥 카메라는 자신이 보는 점 중 이 비율 이상이 파트에 속할 때 그 파트와 함께 학습합니다.\n높이면 빌리는 카메라가 줄어 빨라지고, 낮추면 더 많은 시점이 이음새를 양쪽에서 봅니다."),
    DE("Eine äußere Kamera wird mittrainiert, wenn dieser Anteil ihrer sichtbaren Punkte zum Teil gehört.\nHöher: weniger Kameras, schneller. Niedriger: mehr Ansichten beiderseits der Nähte."),
    FR("Une caméra extérieure s'entraîne avec une partie quand au moins cette part des points qu'elle voit lui appartient.\n- Plus haut : moins de caméras empruntées et des parties plus rapides ;\n- plus bas : des coutures vues des deux côtés par plus de vues."),
    ES("Una cámara externa entrena con una parte cuando al menos esta fracción de los puntos que ve pertenece a la parte.\n- Más alto: menos cámaras prestadas y partes más rápidas;\n- más bajo: costuras vistas desde ambos lados por más vistas."),
    PT("Uma câmara exterior treina com uma parte quando pelo menos esta fração dos pontos que vê lhe pertence.\n- Mais alto: menos câmaras emprestadas e partes mais rápidas;\n- mais baixo: costuras vistas de ambos os lados por mais vistas."),
    IT("Una fotocamera esterna si addestra con una parte quando almeno questa quota dei punti che vede le appartiene.\n- Più alto: meno fotocamere prese in prestito e parti più veloci;\n- più basso: cuciture viste da entrambi i lati da più viste."),
    NL("Een buitencamera traint mee met een deel als minstens dit aandeel van de punten die ze ziet bij het deel hoort.\n- Hoger: minder geleende camera's en snellere delen;\n- lager: naden die door meer beelden van beide kanten worden gezien."),
    RU("Внешняя камера обучается с частью, если хотя бы такая доля видимых ею точек принадлежит части.\nВыше -- меньше заимствованных камер и быстрее части; ниже -- швы видны с обеих сторон большим числом видов."),
    TR("Dış bir kamera, gördüğü noktaların en az bu payı parçaya aitse o parçayla eğitilir.\nYüksek: daha az ödünç kamera ve daha hızlı parçalar; düşük: dikişleri iki yandan gören daha çok görünüm."));

SS_MSG(lbl_seeds,
    EN("Ownership seeds"), JA("所有領域の点"), ZH_HANS("归属种子点"), ZH_HANT("歸屬種子點"),
    KO("소유 시드"), DE("Zugehörigkeitspunkte"), FR("Points d'appartenance"),
    ES("Semillas de pertenencia"), PT("Sementes de pertença"), IT("Semi di appartenenza"),
    NL("Eigendomszaden"), RU("Точки принадлежности"), TR("Aidiyet tohumları"));
SS_MSG(lbl_seeds_help,
    EN("Nearest cloud/camera seed owns each location. More seeds follow scene seams better but increase merge and refinement work."),
    JA("最も近い点群・カメラのシードが各位置を所有します。多いほど境界が形状に沿いますが、結合と精細化の負荷が増えます。"),
    ZH_HANS("每处空间归最近的点云或相机种子所有。种子越多，边界越贴合场景，但增加合并和细化工作量。"),
    ZH_HANT("每處空間歸最近的點雲或相機種子所有。種子越多，邊界越貼合場景，但增加合併和細化工作量。"),
    KO("각 위치는 가장 가까운 점군·카메라 시드에 속합니다. 많을수록 경계가 장면을 잘 따르지만 병합·정제 작업이 늘어납니다."),
    DE("Der nächste Punkt-/Kameraseed bestimmt jeden Ort. Mehr Seeds folgen Szenennähten genauer, erhöhen aber Arbeit bei Fusion und Verfeinerung."),
    FR("Chaque lieu appartient au germe de point/caméra le plus proche. Plus de germes affinent les limites mais coûtent en fusion et affinage."),
    ES("Cada lugar pertenece a la semilla de punto/cámara más cercana. Más semillas afinan los límites, pero aumentan fusión y refinado."),
    PT("Cada local pertence à semente de ponto/câmera mais próxima. Mais sementes refinam limites, mas aumentam fusão e refinamento."),
    IT("Ogni posizione appartiene al seme punto/camera più vicino. Più semi seguono meglio i confini ma aumentano fusione e affinamento."),
    NL("Elke plek hoort bij de dichtstbijzijnde punt-/cameraseed. Meer seeds volgen naden beter, maar kosten meer samenvoeging en verfijning."),
    RU("Каждое место принадлежит ближайшему сиду точки/камеры. Больше сидов точнее задаёт границы, но повышает затраты слияния и уточнения."),
    TR("Her konum en yakın nokta/kamera tohumuna aittir. Çok tohum sınırları daha iyi izler, birleştirme ve iyileştirme işini artırır."));

SS_MSG(btn_compute,
    EN("Compute"), JA("計算"), ZH_HANS("计算"), ZH_HANT("計算"), KO("계산"),
    DE("Berechnen"), FR("Calculer"), ES("Calcular"), PT("Calcular"), IT("Calcola"),
    NL("Berekenen"), RU("Вычислить"), TR("Hesapla"));

SS_MSG(col_part, EN("Part"), JA("パート"), ZH_HANS("分区"), ZH_HANT("分區"), KO("파트"),
    DE("Teil"), FR("Partie"), ES("Parte"), PT("Parte"), IT("Parte"), NL("Deel"),
    RU("Часть"), TR("Parça"));
SS_MSG(col_core, EN("Core"), JA("コア"), ZH_HANS("核心"), ZH_HANT("核心"), KO("핵심"),
    DE("Kern"), FR("Cœur"), ES("Núcleo"), PT("Núcleo"), IT("Nucleo"), NL("Kern"),
    RU("Ядро"), TR("Çekirdek"));
SS_MSG(col_ring, EN("Ring"), JA("リング"), ZH_HANS("外环"), ZH_HANT("外環"), KO("고리"),
    DE("Ring"), FR("Anneau"), ES("Anillo"), PT("Anel"), IT("Anello"), NL("Ring"),
    RU("Кольцо"), TR("Halka"));
SS_MSG(col_points, EN("Points"), JA("点"), ZH_HANS("点"), ZH_HANT("點"), KO("점"),
    DE("Punkte"), FR("Points"), ES("Puntos"), PT("Pontos"), IT("Punti"), NL("Punten"),
    RU("Точки"), TR("Nokta"));
SS_MSG(col_run, EN("Trained model"), JA("学習済みモデル"), ZH_HANS("已训练模型"),
    ZH_HANT("已訓練模型"), KO("학습된 모델"), DE("Trainiertes Modell"),
    FR("Modèle entraîné"), ES("Modelo entrenado"), PT("Modelo treinado"),
    IT("Modello addestrato"), NL("Getraind model"), RU("Обученная модель"),
    TR("Eğitilmiş model"));

SS_MSG(show_all_parts,
    EN("All parts"), JA("すべてのパート"), ZH_HANS("所有分区"), ZH_HANT("所有分區"),
    KO("모든 파트"), DE("Alle Teile"), FR("Toutes les parties"), ES("Todas las partes"),
    PT("Todas as partes"), IT("Tutte le parti"), NL("Alle delen"), RU("Все части"),
    TR("Tüm parçalar"));
SS_MSG(show_help,
    EN("Pick a part to see only what it trains on: its own cameras drawn solid, "
       "its ring drawn faint, the points it seeds from, and the grid cells it "
       "owns."),
    JA("パートを選ぶと、そのパートが学習に使うものだけを表示します。自分のカメラ"
       "は実線、リングは薄く、初期点、所有するグリッドセルです。"),
    ZH_HANS("选择一个分区，只显示它训练所用的内容：自身相机为实线、外环为淡色、它的"
            "种子点，以及它拥有的网格单元。"),
    ZH_HANT("選擇一個分區，只顯示它訓練所用的內容：自身相機為實線、外環為淡色、它的"
            "種子點，以及它擁有的網格單元。"),
    KO("파트를 고르면 그 파트가 학습에 쓰는 것만 보입니다. 자기 카메라는 진하게, "
       "고리는 흐리게, 시드 점, 그리고 소유한 격자 셀입니다."),
    DE("Einen Teil wählen, um nur zu sehen, womit er trainiert: eigene Kameras "
       "kräftig, sein Ring blass, seine Startpunkte und die Gitterzellen, die ihm "
       "gehören."),
    FR("Choisir une partie pour ne voir que ce sur quoi elle s'entraîne : ses "
       "caméras en trait plein, son anneau en pâle, ses points d'amorce et les "
       "cellules qu'elle possède."),
    ES("Elige una parte para ver solo con lo que entrena: sus cámaras en sólido, su "
       "anillo tenue, sus puntos semilla y las celdas que posee."),
    PT("Escolha uma parte para ver só aquilo com que treina: as suas câmaras a "
       "cheio, o seu anel esbatido, os seus pontos semente e as células que possui."),
    IT("Scegli una parte per vedere solo ciò con cui si addestra: le sue fotocamere "
       "piene, il suo anello tenue, i suoi punti seme e le celle che possiede."),
    NL("Kies een deel om alleen te zien waarmee het traint: eigen camera's vol, "
       "zijn ring vaag, zijn zaadpunten en de rastercellen die het bezit."),
    RU("Выберите часть, чтобы видеть только то, на чём она обучается: свои камеры "
       "ярко, кольцо бледно, свои начальные точки и ячейки сетки, которыми владеет."),
    TR("Yalnız neyle eğitildiğini görmek için bir parça seçin: kendi kameraları "
       "dolu, halkası soluk, tohum noktaları ve sahip olduğu ızgara hücreleri."));

SS_MSG(show_owner_colors,
    EN("Colour points by owner"), JA("点を所有パートの色で"), ZH_HANS("按归属给点着色"),
    ZH_HANT("按歸屬給點著色"), KO("점을 소유 파트 색으로"), DE("Punkte nach Besitzer färben"),
    FR("Colorer les points par partie"), ES("Colorear puntos por parte"),
    PT("Colorir pontos por parte"), IT("Colora i punti per parte"),
    NL("Punten kleuren per deel"), RU("Красить точки по части"),
    TR("Noktaları sahibine göre renklendir"));
SS_MSG(show_grid,
    EN("Show regions"), JA("領域を表示"), ZH_HANS("显示区域"), ZH_HANT("顯示區域"),
    KO("영역 표시"), DE("Bereiche zeigen"), FR("Afficher les régions"),
    ES("Mostrar regiones"), PT("Mostrar regiões"), IT("Mostra regioni"),
    NL("Gebieden tonen"), RU("Показать области"), TR("Bölgeleri göster"));
SS_MSG(show_grid_help,
    EN("Colored surfaces with dashed borders show each part’s space and the merge seams, including boundaries through empty space."),
    JA("色付き面と破線で各部分の領域と結合境界を表示します。何もない空間を通る境界も含みます。"),
    ZH_HANS("彩色面及虚线显示各分区范围与合并边界，也包括穿过空白空间的边界。"),
    ZH_HANT("彩色面及虛線顯示各分區範圍與合併邊界，也包括穿過空白空間的邊界。"),
    KO("색칠한 면과 점선으로 각 부분의 공간과 병합 경계를 보여줍니다. 빈 공간을 지나는 경계도 포함합니다."),
    DE("Farbige Flächen mit gestrichelten Rändern zeigen Teilräume und Fusionsnähte, auch durch leeren Raum."),
    FR("Les surfaces colorées à bord pointillé montrent l’espace de chaque partie et les limites de fusion, même dans le vide."),
    ES("Superficies de color con bordes discontinuos muestran el espacio de cada parte y los límites de fusión, incluso en el vacío."),
    PT("Superfícies coloridas com bordas tracejadas mostram o espaço de cada parte e os limites de fusão, inclusive no vazio."),
    IT("Superfici colorate con bordi tratteggiati mostrano lo spazio di ogni parte e i confini di fusione, anche nel vuoto."),
    NL("Gekleurde vlakken met stippelranden tonen deelruimten en samenvoegnaden, ook door lege ruimte."),
    RU("Цветные поверхности с пунктирными краями показывают области частей и границы слияния, включая пустое пространство."),
    TR("Renkli yüzler ve kesikli kenarlar, boş alandan geçenler dahil her parçanın alanını ve birleşme sınırını gösterir."));

SS_MSG(projection_debug,
    EN("Projection debug"), JA("投影デバッグ"), ZH_HANS("投影调试"), ZH_HANT("投影偵錯"),
    KO("투영 디버그"), DE("Projektionsdiagnose"), FR("Diagnostic de projection"),
    ES("Diagnóstico de proyección"), PT("Diagnóstico de projeção"), IT("Diagnostica proiezione"),
    NL("Projectiediagnose"), RU("Отладка проекции"), TR("İzdüşüm tanılama"));
SS_MSG(projection_camera,
    EN("Camera"), JA("カメラ"), ZH_HANS("相机"), ZH_HANT("相機"), KO("카메라"),
    DE("Kamera"), FR("Caméra"), ES("Cámara"), PT("Câmera"), IT("Fotocamera"),
    NL("Camera"), RU("Камера"), TR("Kamera"));
SS_MSG(projection_loading,
    EN("Projecting camera view…"), JA("カメラビューを投影中…"), ZH_HANS("正在生成相机投影…"),
    ZH_HANT("正在產生相機投影…"), KO("카메라 뷰 투영 중…"), DE("Kameraansicht wird projiziert…"),
    FR("Projection de la vue…"), ES("Proyectando la vista…"), PT("Projetando a vista…"),
    IT("Proiezione della vista…"), NL("Cameraweergave projecteren…"), RU("Построение проекции…"),
    TR("Kamera görünümü izdüşürülüyor…"));
SS_MSG(projection_unavailable,
    EN("No projection grid for this camera."), JA("このカメラに投影グリッドはありません。"),
    ZH_HANS("此相机没有投影网格。"), ZH_HANT("此相機沒有投影網格。"), KO("이 카메라의 투영 격자가 없습니다."),
    DE("Kein Projektionsraster für diese Kamera."), FR("Cette caméra n’a pas de grille de projection."),
    ES("Esta cámara no tiene rejilla de proyección."), PT("Esta câmera não tem grade de projeção."),
    IT("Nessuna griglia di proiezione per questa fotocamera."), NL("Geen projectieraster voor deze camera."),
    RU("Для этой камеры нет сетки проекции."), TR("Bu kameranın izdüşüm ızgarası yok."));
SS_MSG(projection_coverage,
    EN("Image coverage"), JA("画像の被覆率"), ZH_HANS("图像覆盖率"), ZH_HANT("影像覆蓋率"),
    KO("이미지 점유율"), DE("Bildabdeckung"), FR("Couverture de l’image"), ES("Cobertura de imagen"),
    PT("Cobertura da imagem"), IT("Copertura dell’immagine"), NL("Beelddekking"),
    RU("Покрытие изображения"), TR("Görüntü kapsamı"));
SS_MSG(projection_sky_coverage,
    EN("Est. sky coverage"), JA("空の推定被覆率"), ZH_HANS("估算天空占比"), ZH_HANT("估算天空佔比"),
    KO("추정 하늘 비율"), DE("Geschätzter Himmelsanteil"), FR("Part de ciel estimée"),
    ES("Cielo estimado"), PT("Céu estimado"), IT("Copertura cielo stimata"), NL("Geschat luchtaandeel"),
    RU("Оценка доли неба"), TR("Tahmini gök oranı"));
SS_MSG(projection_legend,
    EN("Coverage uses valid image pixels. White marks overlap between expanded parts."),
    JA("被覆率は有効な画素に対する割合です。白は拡張した区画の重なりです。"),
    ZH_HANS("覆盖率以有效像素为分母。白色表示分块外扩后的重叠。"),
    ZH_HANT("覆蓋率以有效像素為分母。白色表示分塊外擴後的重疊。"),
    KO("유효한 픽셀 기준입니다. 흰색은 확장된 파트의 겹침입니다."),
    DE("Gezählt werden gültige Bildpixel. Weiß markiert überlappende erweiterte Blöcke."),
    FR("Seuls les pixels valides comptent. Le blanc indique le chevauchement des blocs étendus."),
    ES("La cobertura usa los píxeles válidos. El blanco marca el solape de bloques ampliados."),
    PT("A cobertura usa os pixels válidos. O branco marca a sobreposição dos blocos ampliados."),
    IT("La copertura usa i pixel validi. Il bianco indica la sovrapposizione dei blocchi estesi."),
    NL("Dekking telt geldige pixels. Wit markeert overlap van uitgebreide blokken."),
    RU("Учитываются действительные пиксели. Белым показано пересечение расширенных блоков."),
    TR("Kapsam geçerli piksellere göredir. Beyaz, genişletilmiş blokların örtüşmesini gösterir."));
SS_MSG(projection_mask,
    EN("Projection mask"), JA("投影マスク"), ZH_HANS("投影覆盖图"), ZH_HANT("投影覆蓋圖"),
    KO("투영 마스크"), DE("Projektionsmaske"), FR("Masque de projection"), ES("Máscara de proyección"),
    PT("Máscara de projeção"), IT("Maschera di proiezione"), NL("Projectiemasker"),
    RU("Маска проекции"), TR("İzdüşüm maskesi"));
SS_MSG(projection_photo,
    EN("Source photo"), JA("元の写真"), ZH_HANS("原始照片"), ZH_HANT("原始照片"), KO("원본 사진"),
    DE("Quellfoto"), FR("Photo source"), ES("Foto original"), PT("Foto original"), IT("Foto originale"),
    NL("Bronfoto"), RU("Исходное фото"), TR("Kaynak fotoğraf"));

SS_MSG(lbl_file, EN("File"), JA("ファイル"), ZH_HANS("文件"), ZH_HANT("檔案"), KO("파일"),
    DE("Datei"), FR("Fichier"), ES("Archivo"), PT("Ficheiro"), IT("File"), NL("Bestand"),
    RU("Файл"), TR("Dosya"));
SS_MSG(btn_save,
    EN("Save partition"), JA("分割を保存"), ZH_HANS("保存分区"), ZH_HANT("儲存分區"),
    KO("분할 저장"), DE("Partition speichern"), FR("Enregistrer la partition"),
    ES("Guardar partición"), PT("Guardar partição"), IT("Salva partizione"),
    NL("Partitie opslaan"), RU("Сохранить разбиение"), TR("Bölümlemeyi kaydet"));
SS_MSG(saved_to,
    EN("Saved: {0}"), JA("保存しました: {0}"), ZH_HANS("已保存：{0}"), ZH_HANT("已儲存：{0}"),
    KO("저장했습니다: {0}"), DE("Gespeichert: {0}"), FR("Enregistré : {0}"),
    ES("Guardado: {0}"), PT("Guardado: {0}"), IT("Salvato: {0}"), NL("Opgeslagen: {0}"),
    RU("Сохранено: {0}"), TR("Kaydedildi: {0}"));

SS_MSG(btn_batch,
    EN("Queue parts in Batch"), JA("パートをバッチに追加"), ZH_HANS("把分区排入批处理"),
    ZH_HANT("把分區排入批次處理"), KO("파트를 배치에 넣기"), DE("Teile in den Stapel stellen"),
    FR("Mettre les parties en file dans Lot"), ES("Poner las partes en cola en Lote"),
    PT("Pôr as partes em fila no Lote"), IT("Metti le parti in coda nel Lotto"),
    NL("Delen in Batch zetten"), RU("Поставить части в пакет"),
    TR("Parçaları Toplu İşe sırala"));
SS_MSG(btn_batch_help,
    EN("Save and queue one training run per part in Batch, using its training preset. Merge here after all parts finish."),
    JA("保存し、バッチ画面の学習プリセットで各部分の学習を登録します。全部分の完了後、ここで結合してください。"),
    ZH_HANS("保存并按批处理页面的训练预设，为每个分区排队训练。全部完成后在此合并。"),
    ZH_HANT("儲存並按批次處理頁面的訓練預設，為每個分區排程訓練。全部完成後在此合併。"),
    KO("저장 후 배치 화면의 학습 프리셋으로 부분별 학습을 대기열에 넣습니다. 모두 끝나면 여기서 병합하세요."),
    DE("Speichern und je Teil ein Training in Batch mit dessen Trainingspreset einreihen. Nach Abschluss aller Teile hier zusammenführen."),
    FR("Enregistre et met chaque partie en file dans Lot avec son préréglage d’entraînement. Fusionnez ici quand tout est fini."),
    ES("Guarda y pone cada parte en la cola de Lote con su ajuste de entrenamiento. Fusiona aquí al terminar todas."),
    PT("Salva e coloca cada parte na fila de Lote com sua predefinição de treino. Faça a fusão aqui quando todas terminarem."),
    IT("Salva e accoda un training per parte in Batch con il suo preset. Unisci qui al termine di tutte le parti."),
    NL("Bewaar en zet training per deel in Batch met de trainingspreset daar. Voeg hier samen nadat alle delen klaar zijn."),
    RU("Сохраняет и ставит обучение каждой части в очередь Batch с его пресетом. Объедините здесь после завершения всех частей."),
    TR("Kaydedip her parça için Toplu ekranının eğitim önayarını kullanan işi sıraya koyar. Hepsi bitince burada birleştirin."));
SS_MSG(batch_added,
    EN("Runs queued: {0}. Start them on the Batch screen."),
    JA("学習を {0} 件並べました。バッチ画面で開始してください。"),
    ZH_HANS("已排入 {0} 次训练。请到批处理页面开始。"),
    ZH_HANT("已排入 {0} 次訓練。請到批次處理頁面開始。"),
    KO("학습 {0}개를 넣었습니다. 배치 화면에서 시작하세요."),
    DE("Läufe eingereiht: {0}. Auf dem Stapel-Bildschirm starten."),
    FR("Entraînements en file : {0}. Lancez-les sur l'écran Lot."),
    ES("Ejecuciones en cola: {0}. Inícialas en la pantalla Lote."),
    PT("Treinos em fila: {0}. Inicie-os no ecrã Lote."),
    IT("Esecuzioni in coda: {0}. Avviale nella schermata Lotto."),
    NL("Runs in de wachtrij: {0}. Start ze op het Batch-scherm."),
    RU("Запусков в очереди: {0}. Запустите их на экране пакета."),
    TR("Sıralanan çalıştırma: {0}. Toplu İş ekranından başlatın."));

SS_MSG(merge_head,
    EN("Merge"), JA("結合"), ZH_HANS("合并"), ZH_HANT("合併"), KO("병합"),
    DE("Zusammenführen"), FR("Fusion"), ES("Fusión"), PT("Fusão"), IT("Unione"),
    NL("Samenvoegen"), RU("Объединение"), TR("Birleştirme"));
SS_MSG(lbl_runs,
    EN("Runs folder"), JA("学習出力フォルダ"), ZH_HANS("训练输出文件夹"),
    ZH_HANT("訓練輸出資料夾"), KO("학습 출력 폴더"), DE("Ordner der Läufe"),
    FR("Dossier des entraînements"), ES("Carpeta de ejecuciones"),
    PT("Pasta dos treinos"), IT("Cartella delle esecuzioni"), NL("Map met runs"),
    RU("Папка запусков"), TR("Çalıştırma klasörü"));
SS_MSG(btn_scan,
    EN("Find models"), JA("モデルを探す"), ZH_HANS("查找模型"), ZH_HANT("尋找模型"),
    KO("모델 찾기"), DE("Modelle suchen"), FR("Chercher les modèles"),
    ES("Buscar modelos"), PT("Procurar modelos"), IT("Trova modelli"),
    NL("Modellen zoeken"), RU("Найти модели"), TR("Modelleri bul"));
SS_MSG(btn_scan_help,
    EN("Looks through the runs folder for training runs whose config.json names "
       "this partition file, newest per part."),
    JA("学習出力フォルダから、config.json がこの分割ファイルを指している学習を探"
       "し、パートごとに最新のものを使います。"),
    ZH_HANS("在训练输出文件夹中查找 config.json 指向此分区文件的训练，每个分区取最新的。"),
    ZH_HANT("在訓練輸出資料夾中尋找 config.json 指向此分區檔案的訓練，每個分區取最新的。"),
    KO("학습 출력 폴더에서 config.json이 이 분할 파일을 가리키는 학습을 찾아 파트마다 "
       "가장 새 것을 씁니다."),
    DE("Durchsucht den Ordner der Läufe nach Trainingsläufen, deren config.json "
       "diese Partitionsdatei nennt, je Teil den neuesten."),
    FR("Parcourt le dossier des entraînements à la recherche de ceux dont le "
       "config.json nomme ce fichier de partition, le plus récent par partie."),
    ES("Recorre la carpeta de ejecuciones buscando las que nombran este archivo de "
       "partición en su config.json, la más reciente por parte."),
    PT("Percorre a pasta dos treinos à procura dos que nomeiam este ficheiro de "
       "partição no seu config.json, o mais recente por parte."),
    IT("Scorre la cartella delle esecuzioni cercando quelle il cui config.json "
       "nomina questo file di partizione, la più recente per parte."),
    NL("Doorzoekt de map met runs naar trainingsruns waarvan de config.json dit "
       "partitiebestand noemt, de nieuwste per deel."),
    RU("Просматривает папку запусков в поиске тех, чей config.json называет этот "
       "файл разбиения, по новейшему на часть."),
    TR("Çalıştırma klasöründe config.json'u bu bölümleme dosyasını gösteren "
       "eğitimleri arar, parça başına en yenisini alır."));
SS_MSG(scan_result,
    EN("Parts with a model: {0} of {1}"),
    JA("モデルのあるパート: {1} 中 {0}"),
    ZH_HANS("有模型的分区：{0} / {1}"), ZH_HANT("有模型的分區：{0} / {1}"),
    KO("모델이 있는 파트: {1} 중 {0}"), DE("Teile mit Modell: {0} von {1}"),
    FR("Parties avec un modèle : {0} sur {1}"), ES("Partes con modelo: {0} de {1}"),
    PT("Partes com modelo: {0} de {1}"), IT("Parti con un modello: {0} di {1}"),
    NL("Delen met een model: {0} van {1}"), RU("Частей с моделью: {0} из {1}"),
    TR("Modeli olan parça: {1} içinde {0}"));
SS_MSG(run_missing,
    EN("(none)"), JA("（なし）"), ZH_HANS("（无）"), ZH_HANT("（無）"), KO("(없음)"),
    DE("(keins)"), FR("(aucun)"), ES("(ninguno)"), PT("(nenhum)"), IT("(nessuno)"),
    NL("(geen)"), RU("(нет)"), TR("(yok)"));
SS_MSG(btn_merge,
    EN("Merge models"), JA("モデルを結合"), ZH_HANS("合并模型"), ZH_HANT("合併模型"),
    KO("모델 병합"), DE("Modelle zusammenführen"), FR("Fusionner les modèles"),
    ES("Fusionar modelos"), PT("Fundir modelos"), IT("Unisci modelli"),
    NL("Modellen samenvoegen"), RU("Объединить модели"), TR("Modelleri birleştir"));
SS_MSG(merge_done,
    EN("Merged: {0}   splats {1}"), JA("結合しました: {0}   スプラット {1}"),
    ZH_HANS("已合并：{0}   泼溅 {1}"), ZH_HANT("已合併：{0}   潑濺 {1}"),
    KO("병합했습니다: {0}   스플랫 {1}"), DE("Zusammengeführt: {0}   Splats {1}"),
    FR("Fusionné : {0}   splats {1}"), ES("Fusionado: {0}   splats {1}"),
    PT("Fundido: {0}   splats {1}"), IT("Unito: {0}   splat {1}"),
    NL("Samengevoegd: {0}   splats {1}"), RU("Объединено: {0}   сплатов {1}"),
    TR("Birleştirildi: {0}   splat {1}"));
SS_MSG(btn_open_merged,
    EN("Open in viewer"), JA("ビューアで開く"), ZH_HANS("在查看器中打开"),
    ZH_HANT("在檢視器中開啟"), KO("뷰어에서 열기"), DE("Im Betrachter öffnen"),
    FR("Ouvrir dans la visionneuse"), ES("Abrir en el visor"), PT("Abrir no visualizador"),
    IT("Apri nel visualizzatore"), NL("Openen in de viewer"), RU("Открыть в просмотрщике"),
    TR("Görüntüleyicide aç"));
SS_MSG(stage_merge,
    EN("Merge"), JA("結合"), ZH_HANS("合并"), ZH_HANT("合併"), KO("병합"),
    DE("Zusammenführen"), FR("Fusionner"), ES("Fusionar"), PT("Fundir"), IT("Unisci"),
    NL("Samenvoegen"), RU("Объединить"), TR("Birleştir"));
SS_MSG(plan_merge,
    EN("{0}. Merge the trained parts of {1}"),
    JA("{0}. {1} の学習済みパートを結合する"),
    ZH_HANS("{0}. 合并 {1} 已训练的各分区"),
    ZH_HANT("{0}. 合併 {1} 已訓練的各分區"),
    KO("{0}. {1}의 학습된 파트를 병합"),
    DE("{0}. Die trainierten Teile von {1} zusammenführen"),
    FR("{0}. Fusionner les parties entraînées de {1}"),
    ES("{0}. Fusionar las partes entrenadas de {1}"),
    PT("{0}. Fundir as partes treinadas de {1}"),
    IT("{0}. Unire le parti addestrate di {1}"),
    NL("{0}. De getrainde delen van {1} samenvoegen"),
    RU("{0}. Объединить обученные части {1}"),
    TR("{0}. {1} için eğitilmiş parçaları birleştir"));
SS_MSG(batch_log_merge,
    EN("[{0}] Merging the parts of {1}"),
    JA("[{0}] {1} のパートを結合しています"),
    ZH_HANS("[{0}] 正在合并 {1} 的各分区"),
    ZH_HANT("[{0}] 正在合併 {1} 的各分區"),
    KO("[{0}] {1}의 파트를 병합하는 중"),
    DE("[{0}] Die Teile von {1} werden zusammengeführt"),
    FR("[{0}] Fusion des parties de {1}"),
    ES("[{0}] Fusionando las partes de {1}"),
    PT("[{0}] A fundir as partes de {1}"),
    IT("[{0}] Unione delle parti di {1}"),
    NL("[{0}] De delen van {1} worden samengevoegd"),
    RU("[{0}] Объединение частей {1}"),
    TR("[{0}] {1} parçaları birleştiriliyor"));
SS_MSG(merge_row_help,
    EN("Runs after the rows above it: every part of this partition that has a "
       "trained model in the runs folder is joined into one file written there."),
    JA("上の行のあとに実行されます。この分割のうち、出力フォルダに学習済みモデル"
       "があるパートをすべて結合し、そこにひとつのファイルとして書き出します。"),
    ZH_HANS("在上方各行之后运行：把此分区中所有在输出文件夹里有训练模型的分区合并成"
            "一个文件并写在那里。"),
    ZH_HANT("在上方各列之後執行：把此分區中所有在輸出資料夾裡有訓練模型的分區合併成"
            "一個檔案並寫在那裡。"),
    KO("위 행들 다음에 실행됩니다. 이 분할에서 출력 폴더에 학습된 모델이 있는 모든 "
       "파트를 하나의 파일로 합쳐 그곳에 씁니다."),
    DE("Läuft nach den Zeilen darüber: Jeder Teil dieser Partition, der im "
       "Ordner der Läufe ein trainiertes Modell hat, wird zu einer Datei dort "
       "zusammengeführt."),
    FR("S'exécute après les lignes du dessus : chaque partie de cette partition "
       "ayant un modèle entraîné dans le dossier des entraînements est réunie en "
       "un fichier écrit là."),
    ES("Se ejecuta tras las filas de arriba: cada parte de esta partición con un "
       "modelo entrenado en la carpeta de ejecuciones se une en un archivo "
       "escrito allí."),
    PT("Corre depois das linhas acima: cada parte desta partição com um modelo "
       "treinado na pasta dos treinos é reunida num ficheiro escrito lá."),
    IT("Viene eseguito dopo le righe sopra: ogni parte di questa partizione con "
       "un modello addestrato nella cartella delle esecuzioni è riunita in un "
       "file scritto lì."),
    NL("Draait na de rijen erboven: elk deel van deze partitie met een getraind "
       "model in de map met runs wordt tot één bestand daar samengevoegd."),
    RU("Выполняется после строк выше: каждая часть этого разбиения, у которой в "
       "папке запусков есть обученная модель, объединяется в один файл там же."),
    TR("Üstteki satırlardan sonra çalışır: bu bölümlemenin çalıştırma "
       "klasöründe eğitilmiş modeli olan her parçası orada tek bir dosyada "
       "birleştirilir."));
SS_MSG(btn_open_batch,
    EN("Open Batch Processing"), JA("バッチ処理を開く"), ZH_HANS("打开批处理"),
    ZH_HANT("開啟批次處理"), KO("배치 처리 열기"), DE("Stapelverarbeitung öffnen"),
    FR("Ouvrir le traitement par lot"), ES("Abrir procesamiento por lotes"),
    PT("Abrir processamento em lote"), IT("Apri elaborazione in lotto"),
    NL("Batchverwerking openen"), RU("Открыть пакетную обработку"),
    TR("Toplu işlemeyi aç"));
SS_MSG(pq_title,
    EN("Queue the parts"), JA("パートをキューに入れる"), ZH_HANS("将分区排入队列"),
    ZH_HANT("將分區排入佇列"), KO("파트를 큐에 넣기"), DE("Teile einreihen"),
    FR("Mettre les parties en file"), ES("Poner las partes en cola"),
    PT("Pôr as partes em fila"), IT("Metti le parti in coda"),
    NL("Delen in de wachtrij zetten"), RU("Поставить части в очередь"),
    TR("Parçaları sıraya al"));
SS_MSG(pq_runs,
    EN("Training runs to queue: {0}, one per part of {1}\nSplat and step limits apply to each part."),
    JA("キューに入れる学習: {0} 件（{1} のパートごとに 1 件）\nスプラット数・ステップ数の上限はパートごとです。"),
    ZH_HANS("将排入的训练：{0} 次，{1} 的每个分区一次\n泼溅数量和步数上限分别应用于每个分区。"),
    ZH_HANT("將排入的訓練：{0} 次，{1} 的每個分區一次\n潑濺數量和步數上限分別套用至每個分區。"),
    KO("큐에 넣을 학습: {0}개, {1}의 파트마다 하나\n스플랫 수와 스텝 수 제한은 파트마다 적용됩니다."),
    DE("Einzureihende Trainingsläufe: {0}, einer je Teil von {1}\nSplat- und Schrittlimits gelten pro Teil."),
    FR("Entraînements à mettre en file : {0}, un par partie de {1}\nLes limites de splats et d'étapes valent par partie."),
    ES("Ejecuciones a poner en cola: {0}, una por parte de {1}\nLos límites de splats y pasos se aplican a cada parte."),
    PT("Treinos a pôr em fila: {0}, um por parte de {1}\nOs limites de splats e passos valem para cada parte."),
    IT("Esecuzioni da mettere in coda: {0}, una per parte di {1}\nI limiti di splat e passi valgono per ogni parte."),
    NL("Trainingsruns in de wachtrij: {0}, één per deel van {1}\nSplat- en staplimieten gelden per deel."),
    RU("Запусков обучения в очередь: {0}, по одному на часть {1}\nЛимиты сплатов и шагов задаются для каждой части."),
    TR("Sıraya alınacak eğitim: {0}, {1} parçası başına bir\nSplat ve adım sınırları her parça için geçerlidir."));
SS_MSG(pq_merge,
    EN("Merge the parts once all have trained"),
    JA("すべて学習し終えたらパートを結合する"),
    ZH_HANS("全部训练完成后合并各分区"), ZH_HANT("全部訓練完成後合併各分區"),
    KO("모두 학습되면 파트를 병합"), DE("Die Teile zusammenführen, sobald alle trainiert sind"),
    FR("Fusionner les parties une fois toutes entraînées"),
    ES("Fusionar las partes cuando todas hayan entrenado"),
    PT("Fundir as partes quando todas tiverem treinado"),
    IT("Unire le parti quando tutte sono addestrate"),
    NL("De delen samenvoegen zodra alle getraind zijn"),
    RU("Объединить части, когда все обучены"),
    TR("Hepsi eğitilince parçaları birleştir"));
SS_MSG(pq_merge_help,
    EN("Adds a last row to the list that joins the parts' models into one file "
       "in the runs folder, written beside the dataset."),
    JA("一覧の最後に、各パートのモデルをひとつのファイルに結合してデータセットの隣"
       "の出力フォルダに書き出す行を追加します。"),
    ZH_HANS("在列表末尾加一行，把各分区的模型合并成一个文件，写到数据集旁的输出文件夹。"),
    ZH_HANT("在列表末尾加一列，把各分區的模型合併成一個檔案，寫到資料集旁的輸出資料夾。"),
    KO("목록 끝에 각 파트의 모델을 하나의 파일로 합쳐 데이터셋 옆 출력 폴더에 쓰는 "
       "행을 추가합니다."),
    DE("Hängt eine letzte Zeile an, die die Modelle der Teile zu einer Datei im "
       "Ordner der Läufe neben dem Datensatz zusammenführt."),
    FR("Ajoute une dernière ligne qui réunit les modèles des parties en un fichier "
       "dans le dossier des entraînements, à côté du jeu de données."),
    ES("Añade una última fila que une los modelos de las partes en un archivo en "
       "la carpeta de ejecuciones, junto al conjunto de datos."),
    PT("Acrescenta uma última linha que reúne os modelos das partes num ficheiro "
       "na pasta dos treinos, ao lado do conjunto de dados."),
    IT("Aggiunge un'ultima riga che riunisce i modelli delle parti in un file "
       "nella cartella delle esecuzioni, accanto al set di dati."),
    NL("Voegt een laatste rij toe die de modellen van de delen tot één bestand in "
       "de map met runs naast de dataset samenvoegt."),
    RU("Добавляет последнюю строку, которая объединяет модели частей в один файл "
       "в папке запусков рядом с набором данных."),
    TR("Listeye, parçaların modellerini veri kümesinin yanındaki çalıştırma "
       "klasöründe tek dosyada birleştiren son bir satır ekler."));
SS_MSG(pq_queue,
    EN("Queue"), JA("キューに入れる"), ZH_HANS("排入队列"), ZH_HANT("排入佇列"),
    KO("큐에 넣기"), DE("Einreihen"), FR("Mettre en file"), ES("Poner en cola"),
    PT("Pôr em fila"), IT("Metti in coda"), NL("In de wachtrij"), RU("В очередь"),
    TR("Sıraya al"));
SS_MSG(pq_pending,
    EN("Rows on the batch list that have not finished: {0}. Clear them before "
       "queuing the parts?"),
    JA("バッチ一覧にまだ終わっていない行があります: {0}。パートを入れる前にそれら"
       "を消しますか？"),
    ZH_HANS("批处理列表中还有 {0} 行尚未完成。排入分区之前先清除它们吗？"),
    ZH_HANT("批次處理列表中還有 {0} 列尚未完成。排入分區之前先清除它們嗎？"),
    KO("배치 목록에 아직 끝나지 않은 행이 {0}개 있습니다. 파트를 넣기 전에 지울까요?"),
    DE("Zeilen auf der Stapelliste, die nicht fertig sind: {0}. Vor dem Einreihen "
       "der Teile entfernen?"),
    FR("Lignes de la liste non terminées : {0}. Les retirer avant de mettre les "
       "parties en file ?"),
    ES("Filas de la lista que no han terminado: {0}. ¿Eliminarlas antes de poner "
       "las partes en cola?"),
    PT("Linhas da lista que não terminaram: {0}. Removê-las antes de pôr as "
       "partes em fila?"),
    IT("Righe della lista non terminate: {0}. Rimuoverle prima di mettere le "
       "parti in coda?"),
    NL("Rijen op de batchlijst die niet klaar zijn: {0}. Verwijderen voordat de "
       "delen in de wachtrij gaan?"),
    RU("Незавершённых строк в пакетном списке: {0}. Убрать их перед постановкой "
       "частей в очередь?"),
    TR("Toplu listede bitmemiş satır: {0}. Parçaları sıraya almadan önce "
       "temizlensin mi?"));
SS_MSG(pq_clear,
    EN("Clear them, then queue"), JA("消してから入れる"), ZH_HANS("清除后再排入"),
    ZH_HANT("清除後再排入"), KO("지우고 넣기"), DE("Entfernen, dann einreihen"),
    FR("Les retirer, puis mettre en file"), ES("Eliminarlas y poner en cola"),
    PT("Removê-las e pôr em fila"), IT("Rimuoverle, poi in coda"),
    NL("Verwijderen, dan in de wachtrij"), RU("Убрать и поставить"),
    TR("Temizle, sonra sıraya al"));
SS_MSG(pq_keep,
    EN("Keep them, queue after"), JA("残したまま後ろに入れる"), ZH_HANS("保留并排在其后"),
    ZH_HANT("保留並排在其後"), KO("남겨 두고 뒤에 넣기"), DE("Behalten, danach einreihen"),
    FR("Les garder, mettre en file après"), ES("Conservarlas y poner detrás"),
    PT("Mantê-las e pôr a seguir"), IT("Tenerle, in coda dopo"),
    NL("Bewaren, erachter in de wachtrij"), RU("Оставить, поставить после"),
    TR("Bırak, arkasına sıraya al"));

SS_MSG(legend,
    EN("Frusta take their part's colour: solid for a part's own cameras, faint "
       "for the ring it borrows. Points take their owner's colour."),
    JA("フラスタムは所属パートの色で表示します。パート自身のカメラは実線、借りる"
       "リングは薄く。点は所有パートの色です。"),
    ZH_HANS("视锥按所属分区着色：自身相机为实线，借用的外环为淡色。点按归属分区着色。"),
    ZH_HANT("視錐按所屬分區著色：自身相機為實線，借用的外環為淡色。點按歸屬分區著色。"),
    KO("절두체는 파트 색을 띱니다. 파트 자신의 카메라는 진하게, 빌린 고리는 흐리게. "
       "점은 소유 파트 색입니다."),
    DE("Frusta tragen die Farbe ihres Teils: kräftig für eigene Kameras, blass für "
       "den geliehenen Ring. Punkte tragen die Farbe ihres Besitzers."),
    FR("Les frustums prennent la couleur de leur partie : plein pour ses propres "
       "caméras, pâle pour l'anneau emprunté. Les points prennent la couleur de "
       "leur partie."),
    ES("Los frustos toman el color de su parte: sólido para sus propias cámaras, "
       "tenue para el anillo prestado. Los puntos toman el color de su parte."),
    PT("Os frustos tomam a cor da sua parte: cheio para as próprias câmaras, "
       "esbatido para o anel emprestado. Os pontos tomam a cor da sua parte."),
    IT("I frustum prendono il colore della loro parte: pieno per le proprie "
       "fotocamere, tenue per l'anello preso in prestito. I punti prendono il "
       "colore della loro parte."),
    NL("Frusta krijgen de kleur van hun deel: vol voor eigen camera's, vaag voor "
       "de geleende ring. Punten krijgen de kleur van hun eigenaar."),
    RU("Пирамиды видимости окрашены по части: ярко -- свои камеры, бледно -- "
       "заимствованное кольцо. Точки окрашены по владельцу."),
    TR("Görüş piramitleri parçasının rengini alır: kendi kameraları dolu, ödünç "
       "halka soluk. Noktalar sahibinin rengini alır."));

}  // namespace partition
}  // namespace msg
}  // namespace i18n
}  // namespace spirula

#include "i18n/EndCatalog.h"
