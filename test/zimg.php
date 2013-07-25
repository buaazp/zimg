<html>
<body>
<?php

function convert($image, $width, $height, $propor, $gray) 
{
    $rst = -1;
    $imagick = new Imagick();
    $imagick->readImage($image);
    $w = $imagick->getImageWidth();
    $h = $imagick->getImageHeight();
    //echo 'w = ' . $w . '<br>h = ' . $h . '<br>';
    if($gray == '1')
    {
        $imagick->setImageColorspace(imagick::COLORSPACE_GRAY);
    }
    if($width == '0' && $height == '0')
    {
        //$imagick->setImageFormat(‘JPEG’);
        $imagick->setImageCompression(Imagick::COMPRESSION_JPEG);
        $a = $imagick->getImageCompressionQuality() * 0.75;
        if ($a == 0) {
            $a = 75;
        }
        $imagick->setImageCompressionQuality($a);
        $imagick->stripImage();
        $imagick->writeImage('new.jpeg');
        $rst = 1;
    }
    else if(($width != '0' && $w > $width) || ($height != '0' && $h > $height)) 
    {
        if($propor == '0')
        {
            if($width == '0' || $height == '0')
            {
                $rst = -1;
                $imagick->clear();
                $imagick->destroy();
                return $rst;
            }
            else
            {
                //echo 'width = ' . $width . '<br>height = ' . $height . '<br>';
                $imagick->resizeImage($width, $height, Imagick::FILTER_LANCZOS, 1, false);
            }
        }
        else
        {
            if($width == '0')
            {
                $width = $height * $w / $h;
            }
            if($height == '0')
            {
                $height = $width * $h / $w;
            }
            //echo 'width = ' . $width . '<br>height = ' . $height . '<br>';
            $imagick->resizeImage($width, $height, Imagick::FILTER_LANCZOS, 1, true);
        }
        //$imagick->setImageFormat(‘JPEG’);
        $imagick->setImageCompression(Imagick::COMPRESSION_JPEG);
        $a = $imagick->getImageCompressionQuality() * 0.75;
        if ($a == 0) {
            $a = 75;
        }
        $imagick->setImageCompressionQuality($a);
        $imagick->stripImage();
        $imagick->writeImage('new.jpeg');
        $rst = 1;
    }
    else if($gray == '1')
    {
        //$imagick->setImageFormat(‘JPEG’);
        $imagick->setImageCompression(Imagick::COMPRESSION_JPEG);
        $a = $imagick->getImageCompressionQuality() * 0.75;
        if ($a == 0) {
            $a = 75;
        }
        $imagick->setImageCompressionQuality($a);
        $imagick->stripImage();
        $imagick->writeImage('new.jpeg');
        $rst = 1;
    }
    else
    {
        $rst = 2;
    }

    $imagick->clear();
    $imagick->destroy();
    return $rst;
}

if(is_array($_GET) && count($_GET)>0)
{
    if(isset($_GET["md5"]))
    {
        $md5 = $_GET[md5];
        if(isset($_GET["w"]))
        {
            $width = $_GET[w];
        }
        else
        {
            $width = '0';
        }
        if(isset($_GET["h"]))
        {
            $height = $_GET[h];
        }
        else
        {
            $height = '0';
        }
        if(isset($_GET["p"]))
        {
            $propor = $_GET[p];
        }
        else
        {
            $propor = '1';
        }
        if(isset($_GET["g"]))
        {
            $gray = $_GET[g];
        }
        else
        {
            $gray = '0';
        }

        $file_name = './'. $md5 . '.jpeg';
        /* echo $file_name . '<br>'; */
        if($width == '0' && $height == '0' && $gray == '0')
        {
            echo "<img src=" . $file_name . ">";
        }
        else
        {
            $rst = convert($file_name, $width, $height, $propor, $gray);
            //echo 'rst = ' . $rst . '<br>';
            if($rst == 1)
            {
                echo "<img src=./new.jpeg>";
            }
            else if($rst == 2)
            {
                echo "<img src=" . $file_name . ">";
            }
            else
            {
                echo "<h1>404 Not Found!</h1>";
            }
        }
    }
    else
    {
        echo "<h1>404 Not Found!</h1>";
    }
}
else
{
    echo "<h1>404 Not Found!</h1>";
}


?>

</body>
</html>
