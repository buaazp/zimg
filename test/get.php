<?php

$imagick = new Imagick();

function convert($image, $width, $height, $propor, $gray) 
{
    $rst = -1;
    //echo $image . '<br>';
    global $imagick;
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
        $imagick->writeImage($width . '_' . $height . '_' . $propor .'_' . $gray);
        $rst = 1;
    }
    else if(($width != '0' && $w > $width) || ($height != '0' && $h > $height)) 
    {
        if($propor == '0')
        {
            if($width == '0' || $height == '0')
            {
                $rst = -1;
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
        $imagick->writeImage($width . '_' . $height . '_' . $propor .'_' . $gray);
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
        $imagick->writeImage($width . '_' . $height . '_' . $propor .'_' . $gray);
        $rst = 1;
    }
    else
    {
        $rst = 2;
    }

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
        //echo $file_name . '<br>'; 
        if($width == '0' && $height == '0' && $gray == '0')
        {
            $imagick->readImage($file_name);
            header( "Content-Type: image/jpeg" );
            echo $imagick;
        }
        else
        {
            $rst = convert($file_name, $width, $height, $propor, $gray);
            //echo 'rst = ' . $rst . '<br>';
            if($rst == 1)
            {
                header( "Content-Type: image/jpeg" );
                echo $imagick;
            }
            else if($rst == 2)
            {
                $imagick->readImage($file_name);
                header( "Content-Type: image/jpeg" );
                echo $imagick;
            }
            else
            {
                header( "Content-Type: text/html" );
                echo "<html><body><h1>404 Not Found!</h1></body></html>";
            }
        }
    }
    else
    {
        header( "Content-Type: text/html" );
        echo "<html><body><h1>404 Not Found!</h1></body></html>";
    }
}
else
{
    echo "<h1>404 Not Found!</h1>";
}


$imagick->clear();
$imagick->destroy();
?>

